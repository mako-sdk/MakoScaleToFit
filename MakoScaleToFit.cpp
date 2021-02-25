// -----------------------------------------------------------------------
//  <copyright file="MakoScaleToFit.cpp" company="Global Graphics Software Ltd">
//      Copyright (c) 2021 Global Graphics Software Ltd. All rights reserved.
//  </copyright>
//  <summary>
//  This example is provided on an "as is" basis and without warranty of any kind.
//  Global Graphics Software Ltd. does not warrant or make any representations
//  regarding the use or results of use of this example.
//  </summary>
// -----------------------------------------------------------------------

#include <exception>
#include <iostream>
#include <jawsmako/jawsmako.h>
#include <jawsmako/pdfinput.h>
#include <jawsmako/pdfoutput.h>
#include <edl/idommetadata.h>
#include <iostream>
#include <map>

#if defined(_WIN32)
#include <filesystem>
namespace fs = std::filesystem;
#elif defined(__GNUC__)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#elif defined(__APPLE__)
#include <filesystem>
namespace fs = std::__fs::filesystem;
#endif

#include "MakoPageSizes.h"

#define XPS2PDF(value) value / 96.0 * 72.0
#define PDF2XPS(value) value / 72.0 * 96.0

using namespace JawsMako;
using namespace EDL;
namespace fs = std::filesystem;

// Print usage
static void usage(PageSizes pageSizes)
{
    std::wcerr << L"Usage: <source folder> [<page size>]"; // required arguments

    std::wcout << L"   Where: Page size chosen from the list below. Default is US Letter (8.5in x 11in)." << std::endl;
    std::wcout << std::endl;
	
    uint8 colCount = 0;
    PageSizes::iterator it = pageSizes.begin();
    while (it != pageSizes.end())
    {
        String name = it->first;
        std::wcout << name << L"\t";
        if (name.size() < 16) {
            std::wcout << L"\t";
        }
        if (name.size() < 8) {
            std::wcout << L"\t";
        }
        ++it;
        if (++colCount == 4)
        {
            std::wcout << std::endl;
            colCount = 0;
        }
    }
}

#ifdef _WIN32
int wmain(int argc, wchar_t* argv[])
#else
int main(int argc, char* argv[])
#endif
{
	try
    {
	    PageSizes pageSizes = GetPageSizeList();
	    IJawsMakoPtr jawsMako = IJawsMako::create();
	    IJawsMako::enableAllFeatures(jawsMako);

	    // There must be at least one argument, the input folder
	    if (argc < 2)
	        usage(pageSizes);

	    // Input folder
	#ifdef _WIN32
	    String inputFolder = argv[1];
	#else
	    String inputFolder = U8StringToString(U8String(argv[1]));
	#endif

	    if (!fs::exists(inputFolder) || fs::is_empty(inputFolder))
	    {
	        std::wcerr << L"Input folder does not exist or is empty." << std::endl;
	        usage(pageSizes);
	        return 1;
	    }

        // Create output folder
        fs::path inputFolderPath = fs::path(inputFolder);
        fs::path outputFolderPath = inputFolderPath / fs::path(L"out");
		if (!exists(outputFolderPath))
			create_directory(outputFolderPath);

	    // Page size
	    auto requestedPageSize = pageSizes.find(L"LETTER")->second;
		if (argc > 2)
		{
#ifdef _WIN32
			String requiredPageSize = argv[2];
#else
			String requiredPageSize = U8StringToString(U8String(argv[2]));
#endif
			std::transform(requiredPageSize.begin(), requiredPageSize.end(), requiredPageSize.begin(), towupper);
			PageSizes::iterator it = pageSizes.find(requiredPageSize);
			if (it != pageSizes.end())
				requestedPageSize = it->second;
			else
			{
				std::wcerr << L"Page size not recognized." << std::endl;
				usage(pageSizes);
				return 1;
			}
		}

		// For each file found
        for (auto& entry : fs::directory_iterator(inputFolder))
        {
            if (entry.is_regular_file())
            {
                auto inputFile = entry.path();
            	if (inputFile.extension() == ".pdf")
            	{
                    std::wcout << L"Processing: " << inputFile.filename().c_str() << std::endl;

            		// Input
            		auto pdfInput = IPDFInput::create(jawsMako);
                    IDocumentAssemblyPtr assembly = pdfInput->open(inputFile.c_str());
                    IDocumentPtr document = assembly->getDocument();

                    // For each page
            		for (uint32 pageIndex = 0; pageIndex < document->getNumPages(); pageIndex++)
            		{
            			std::wcout << L"  Beginning page " << pageIndex + 1 << L"..." << std::endl;

            			// Set target page size to that requested
                        auto targetPageSize = requestedPageSize;

                        // Grab the content
                        IPagePtr page = document->getPage(pageIndex);
                        double width = page->getWidth();
                        double height = page->getHeight();
                        if (width > height)
                            std::swap(targetPageSize.width, targetPageSize.height);

                        // Prepare to edit
                        IDOMFixedPagePtr content = page->edit();

            			// Set page dimensions to requested page size
                        content->setWidth(targetPageSize.width);
                        content->setHeight(targetPageSize.height);

						// Ensure cropbox (and other dimension boxes) match the new page size
            			FRect cropBox = FRect(0.0, 0.0, targetPageSize.width, targetPageSize.height);
                        content->setCropBox(cropBox);
                        content->setBleedBox(cropBox);
                        content->setTrimBox(cropBox);
                        content->setContentBox(cropBox);
            			
                        // Scale to new page size
            			// The aim is to scale the content to fit while maintaining the correct proportions
            			// And to centre the content horizontally & vertically, as required.
                        double resizeScale = 1.0;
                        double xScaleSize = targetPageSize.width / width;
                        double yScaleSize = targetPageSize.height / height;

                        // Keep the smaller
            			resizeScale = xScaleSize < yScaleSize ? xScaleSize : yScaleSize;

                        // Apply the changes to the page
                        width *= resizeScale;
                        height *= resizeScale;

            			// To center content on page
                        double dx = (targetPageSize.width - width) / 2.0;
                        double dy = (targetPageSize.height - height) / 2.0;

            			// Scale the page content by moving it into a group with a scaling matrix specified
                        IDOMGroupPtr scaleGroup = IDOMGroup::create(
                            jawsMako, FMatrix(resizeScale, 0.0, 0.0, resizeScale, dx, dy));
                        IDOMNodePtr child = content->getFirstChild();
                        while (child)
                        {
                            IDOMNodePtr next = child->getNextSibling();
                            content->extractChild(child);
                            scaleGroup->appendChild(child);
                            child = next;
                        }

            			// Add the content to the page
                        content->appendChild(scaleGroup);
                    }

            		// Output
                    String outFilepath = fs::path(outputFolderPath / entry.path().stem()).c_str();
                    String outputFile = outFilepath + L"_out.pdf";
                    IPDFOutput::create(jawsMako)->writeAssembly(assembly, outputFile);
            	}
            }
        }
	    // Done
    }
    catch (IError& e)
    {
        String errorFormatString = getEDLErrorString(e.getErrorCode());
        std::wcerr << L"Exception thrown: " << e.getErrorDescription(errorFormatString) << std::endl;
#ifdef _WIN32
        // On windows, the return code allows larger numbers, and we can return the error code
        return e.getErrorCode();
#else
        // On other platforms, the exit code is masked to the low 8 bits. So here we just return
        // a fixed value.
        return 1;
#endif
    }
    catch (std::exception& e)
    {
        std::wcerr << L"std::exception thrown: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}