#pragma once

void ETB_Convert(const Eigen::MatrixXr& src, cv::Mat& dst)
{
	int n_rows = (int)src.rows();
	int n_cols = (int)src.cols();
	IKAssert(n_rows == n_cols);
	dst.create(n_rows, n_cols, CV_16U);
	for (int i_row = 0; i_row < n_rows; i_row++)
	{
		for (int i_col = 0; i_col < n_cols; i_col++)
		{
			Real err_ij = src(i_row, i_col);
			const Real err_max = (Real)1;
			const Real err_min = (Real)0;
			err_ij = std::min(err_max, std::max(err_min, err_ij));
			dst.at<unsigned short>(i_row, i_col) = (unsigned short)(err_ij*(Real)USHRT_MAX);
		}
	}
}

void ETB_Convert(const cv::Mat& a_src, Eigen::MatrixXr& dst)
{
	cv::Mat src;
	a_src.convertTo(src, CV_16U);
	const Real USHRT_MAX_INV = (Real)1 / (Real)USHRT_MAX;
	int n_rows = src.rows;
	int n_cols = src.cols;
	IKAssert(n_rows == n_cols);
	dst.resize(n_rows, n_cols);
	for (int i_row = 0; i_row < n_rows; i_row++)
		for (int i_col = 0; i_col < n_cols; i_col++)
			dst(i_row, i_col) = (Real)(src.at<unsigned short>(i_row, i_col))*USHRT_MAX_INV;
}