

template<typename LAMBDA_onext>
void TraverseDirTree(const std::string& dirPath, LAMBDA_onext onbvh, const std::string& ext) //  throw (std::string)
{
	namespace fs = std::experimental::filesystem;
	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;
	std::string filter = dirPath + "\\*";
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError=0;

	// Find the first file in the directory.
	hFind = FindFirstFile(filter.c_str(), &ffd);
	if (INVALID_HANDLE_VALUE == hFind)
	{
		throw std::string("FindFirstFile: ") + dirPath;
	}
	// List all the files in the directory with some info about them.
	std::set<std::string> trivial_dir = {".", ".."};
	bool traversing = true;
	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			continue;
		else if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			bool trivial = (trivial_dir.end() != trivial_dir.find(ffd.cFileName));

			if (!trivial)
			{
				try
				{
					fs::path dirPath_prime = fs::path(dirPath)/ffd.cFileName;
					TraverseDirTree(dirPath_prime.u8string(), onbvh, ext);
				}
				catch (std::string& info)
				{
					FindClose(hFind);
					traversing = false;
					throw info;
				}
			}
		}
		else
		{
			filesize.LowPart = ffd.nFileSizeLow;
			filesize.HighPart = ffd.nFileSizeHigh;

			if (fs::path(ffd.cFileName).extension().u8string() == ext)
			{
				fs::path filepath = fs::path(dirPath)/ffd.cFileName;
				traversing = onbvh(filepath.u8string().c_str());
			}

		}
	}
	while (traversing
		&& FindNextFile(hFind, &ffd) != 0);

	FindClose(hFind);
	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
	{
		throw std::string("FindFirstFile");
	}
}