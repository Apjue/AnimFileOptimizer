// Copyright (C) 2019 Samy Bensaid
// This file is part of the AnimFileMaker project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Nazara/Utility.hpp>
#include <Nazara/Core.hpp>
#include <array>
#include <vector>
#include <unordered_map>
#include <limits>
#include <set>
#include <iostream>
#include <chrono>
#include "imageutil.hpp"

unsigned MaxVerticalPixelsBetweenYLevels { 7 };
constexpr const char* DefaultInputFile { "../file.png" };
constexpr const char* DefaultOutputFile { "../output.png" };

template<class T>
struct CustomRectLess
{
    bool operator()(const Nz::Rect<T>& lhs, const Nz::Rect<T>& rhs) const
    {
        if ((lhs.y > rhs.y ? lhs.y - rhs.y : rhs.y - lhs.y) < T(MaxVerticalPixelsBetweenYLevels)) // If they are on the same Y level
            return lhs.x < rhs.x;

        return lhs.y < rhs.y;
    }
};

int main()
{
    Nz::Initializer<Nz::Utility> nazara;

    if (!nazara)
    {
        std::cerr << "Nazara can't initialize properly. Exiting..." << std::endl;
        return -1;
    }

    NazaraNotice("=====");
    NazaraNotice("Animation File Maker");

    Nz::String inputFilepath;
    Nz::String outputFilepath;
    long long pixelCount {};

    // Load image
    {
        NazaraNotice("Filepathes are relative to the current folder");
        std::cout << "Filepath of the input image [../file.png]: " << std::flush;
        std::getline(std::cin, inputFilepath);

        std::cout << "Filepath of the output image [../output.png]: " << std::flush;
        std::getline(std::cin, outputFilepath);

        Nz::String pixels;
        std::cout << "Max vertical pixels between Y levels [" << MaxVerticalPixelsBetweenYLevels <<"]: " << std::flush;
        std::getline(std::cin, pixels);

        if (pixels.IsNumber())
            pixels.ToInteger(&pixelCount);
    }

    inputFilepath = (inputFilepath == "" ? DefaultInputFile : inputFilepath);
    outputFilepath = (outputFilepath == "" ? DefaultOutputFile : outputFilepath);
    MaxVerticalPixelsBetweenYLevels = (pixelCount > 0 && pixelCount < std::numeric_limits<unsigned>::max() ? pixelCount : MaxVerticalPixelsBetweenYLevels);

    if (!Nz::File::Exists(inputFilepath))
    {
        std::cerr << "File " << inputFilepath << " not found. Exiting...";
        return 1;
    }

    auto start = std::chrono::high_resolution_clock::now();

    Nz::Image inputImage;
    inputImage.LoadFromFile(inputFilepath);
    Nz::Vector3ui size = inputImage.GetSize();


    NazaraNotice("1) Computing labels");
    std::vector<unsigned> labels = getLabelsFromImage(inputImage);


    NazaraNotice("2) Retrieving AABBs");
    std::unordered_map<unsigned, Nz::Vector2ui> labelNumber = getLabelNumber(labels, { size.x, size.y });
    std::map<Nz::Rectui, unsigned, CustomRectLess<unsigned int>> sortedLabels;

    for (auto it = labelNumber.begin(); it != labelNumber.end(); ++it)
        sortedLabels[visitLabel(labels, it->first, it->second, Nz::Vector2ui { size.x, size.y })] = it->first;

    // Shouldn't change anything, but just in case
    sortedLabels.erase(Nz::Rectui { std::numeric_limits<unsigned>::max(), std::numeric_limits<unsigned>::max(), 0u, 0u });

    Nz::Vector2ui maxRectSize {};
    Nz::Vector2ui spriteNumber {};

    {
        bool stopCountingXSprites { false };
        const Nz::Rectui* previousRect {};

        for (auto it = sortedLabels.begin(); it != sortedLabels.end(); ++it)
        {
            const Nz::Rectui& rect = it->first;

            if (previousRect)
            {
                if ((previousRect->y > rect.y ? previousRect->y - rect.y : rect.y - previousRect->y) < MaxVerticalPixelsBetweenYLevels)
                {
                    if (!stopCountingXSprites)
                        ++spriteNumber.x;
                }

                else
                {
                    ++spriteNumber.y;
                    stopCountingXSprites = true;
                }
            }

            else
                ++spriteNumber.x;

            previousRect = &rect;

            maxRectSize.x = std::max(maxRectSize.x, rect.width);
            maxRectSize.y = std::max(maxRectSize.y, rect.height);
        }
    }
    ++spriteNumber.y;


    NazaraNotice("3) Creating recropped image");
    Nz::Vector2ui newSize = maxRectSize * spriteNumber;

    Nz::Image output;
    output.Create(Nz::ImageType_2D, Nz::PixelFormatType_RGBA8, newSize.x, newSize.y);
    output.Fill(Nz::Color { 0, 0, 0, 0 });

    Nz::Vector2ui pos {};

    for (auto it = sortedLabels.begin(); it != sortedLabels.end(); ++it)
    {
        output.Copy(inputImage, it->first, pos + Nz::Vector2ui { (maxRectSize.x - it->first.width) / 2, (maxRectSize.y - it->first.height) / 2 });

        pos.x += maxRectSize.x;

        if (pos.x >= (maxRectSize.x * spriteNumber.x))
        {
            pos.x = 0;
            pos.y += maxRectSize.y;
        }
    }

    output.SaveToFile(outputFilepath);
    auto end = std::chrono::high_resolution_clock::now();

    Nz::Vector3ui finalSize = output.GetSize();
    NazaraNotice(Nz::String { "Saved to "}.Append(outputFilepath).Append(". Dimensions: " ).Append(Nz::String::Number(size.x)).Append("x").Append(Nz::String::Number(size.y)));
    NazaraNotice(Nz::String { "Character dimensions: " }.Append(Nz::String::Number(maxRectSize.x)).Append("x").Append(Nz::String::Number(maxRectSize.y)));
    NazaraNotice(Nz::String { "Elapsed time: " }.Append(Nz::String::Number(std::chrono::duration_cast<std::chrono::seconds>(end - start).count()))
                 .Append("s"));

    return 0;
}
