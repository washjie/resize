#pragma once

#include <iostream>
#include <vector>
#include <utility>
#include <map>
#include <opencv2/opencv.hpp>

//segment 0.5 500 20 input.ppm output.ppm
//run_saliency.m
int width, height;

struct my_comp
{
	bool operator()(const cv::Vec3b& a, const cv::Vec3b& b) const
	{
		return (a[0]<b[0]) || ((a[0] == b[0]) && (a[1]<b[1])) || ((a[0] == b[0]) && (a[1] == b[1]) && (a[2]<b[2]));
	}
};

cv::Mat significance(char* segmentation, char* significance)
{
	cv::Mat seg = cv::imread(segmentation, cv::IMREAD_COLOR);
	cv::Mat sal = cv::imread(significance, cv::IMREAD_GRAYSCALE);

	cv::Mat nothing;

	if (!seg.data || !sal.data)
	{
		std::cerr << "Error: failed to read significant file and segment file" << std::endl;
		return nothing;
	}


	std::map<cv::Vec3b, std::vector< std::pair<int, int> >, my_comp > patch;
	for (int i = 0, height = seg.rows; i < height; i++)
	{
		for (int j = 0, width = seg.cols; j < width; j++)
		{
			patch[seg.at<cv::Vec3b>(i, j)].push_back(std::pair<int, int>(i, j));
		}
	}

	cv::Mat output = sal.clone();
	output.convertTo(output, CV_32F);
	
	
	if (!output.data)
	{
		std::cerr << "no data in the mat!!" << std::endl;
	}

	float pmax = 0, pmin = 300;

	for (auto it = patch.begin(); it != patch.end(); ++it)
	{
		float new_val = .0f;
		for (auto jt = it->second.begin(); jt != it->second.end(); ++jt)
		{
			new_val += (float)sal.at<uchar>(jt->first, jt->second);
		}
		new_val /= (float)it->second.size();

		if (new_val > pmax) pmax = new_val;

		if (new_val < pmin) pmin = new_val;

		for (auto jt = it->second.begin(); jt != it->second.end(); ++jt)
		{
			output.at<float>(jt->first, jt->second) = new_val;
		}
	}
	//std::cout << pmax << " " << pmin << std::endl;

	//normalization (val-pmin)/(pmax-pmin) = (?-0.1)/(1-0.1)
	float base = pmax - pmin;
	for (int i = 0; i < output.rows; ++i)
	{
		for (int j = 0; j < output.cols; ++j)
		{
			output.at<float>(i, j) = (float)(((output.at<float>(i, j) - pmin) / base) * 0.9) + 0.1;
			if (output.at<float>(i, j) > 1 || output.at<float>(i, j) < 0.1)
				std::cout << "normalizztion failed" << std::endl;
		}
	}
	
	return output;
}