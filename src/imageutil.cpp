// Copyright (C) 2019 Samy Bensaid
// This file is part of the AnimFileMaker project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "imageutil.hpp"

std::vector<unsigned> getLabelsFromImage(const Nz::Image& img)
{
    auto size = img.GetSize();

    std::unordered_map<unsigned /* label */, std::pair<Nz::Vector2ui /*removethis*/, std::set<unsigned /* linked labels */>>> linked;
    std::vector<unsigned> labels(size.x * size.y, 0u);
    unsigned nextLabel = 1;

    auto isBackground = [&img] (unsigned x, unsigned y) -> bool { return img.GetPixelColor(x, y).a == 0; };
    auto getLabel = [&labels, &size] (unsigned x, unsigned y) -> unsigned { return labels[y * size.x + x]; };

    auto getNeighbors = [&size, &isBackground] (unsigned x, unsigned y) -> std::vector<Nz::Vector2ui>
    {
        std::vector<Nz::Vector2ui> neighbors;

        if (x < size.x && !isBackground(x + 1, y))
            neighbors.push_back({ x + 1, y });
        if (y < size.y && !isBackground(x, y + 1))
            neighbors.push_back({ x, y + 1 });

        if (x > 0 && !isBackground(x - 1, y))
            neighbors.push_back({ x - 1, y });
        if (y > 0 && !isBackground(x, y - 1))
            neighbors.push_back({ x, y - 1 });


        if (x > 0 && y > 0 && !isBackground(x - 1, y - 1))
            neighbors.push_back({ x - 1, y - 1 });
        if (x > 0 && y < size.y && !isBackground(x - 1, y + 1))
            neighbors.push_back({ x - 1, y + 1 });
        if (x < size.x && y > 0 && !isBackground(x + 1, y - 1))
            neighbors.push_back({ x + 1, y - 1 });
        if (x < size.x && y < size.y && !isBackground(x + 1, y + 1))
            neighbors.push_back({ x + 1, y + 1 });

        return neighbors;
    };

    auto getLabelNeighbors = [&size, &getLabel, &getNeighbors] (unsigned x, unsigned y) -> std::vector<Nz::Vector2ui>
    {
        std::vector<Nz::Vector2ui> neighbors = getNeighbors(x, y);
        std::vector<Nz::Vector2ui> labelNeighbors;
        unsigned label = getLabel(x, y);

        for (auto& neighbor : neighbors)
            if (label == getLabel(neighbor.x, neighbor.y))
                labelNeighbors.push_back(neighbor);

        return labelNeighbors;
    };

    // First pass
    NazaraNotice("1)a) First pass");

    for (unsigned y {}; y < size.y; ++y)
        for (unsigned x {}; x < size.x; ++x)
            if (!isBackground(x, y))
            {
                auto neighbors = getNeighbors(x, y);

                if (neighbors.empty())
                {
                    linked.insert(std::make_pair(nextLabel, std::make_pair(Nz::Vector2ui { x, y }, std::set<unsigned> {})));
                    labels[y * size.x + x] = nextLabel;
                    ++nextLabel;
                }

                else
                {
                    // Find smallest label
                    unsigned smallestLabel { nextLabel };

                    for (auto& neighbor : neighbors)
                    {
                        auto label = getLabel(neighbor.x, neighbor.y);
                        if (label < smallestLabel && label != 0)
                            smallestLabel = label;
                    }

                    if (smallestLabel == nextLabel)
                        ++nextLabel;

                    labels[y * size.x + x] = smallestLabel;

                    for (auto& neighbor : neighbors)
                    {
                        unsigned neighborLabel = getLabel(neighbor.x, neighbor.y);

                        if (neighborLabel == smallestLabel)
                            continue;

                        if (neighborLabel != 0)
                        {
                            auto& data = linked[neighborLabel];
                            data.second.insert(smallestLabel);
                        }

                        else
                            labels[neighbor.y * size.x + neighbor.x] = smallestLabel;
                    }
                }
            }

    // Second pass
    NazaraNotice("1)b) Second pass");
    std::unordered_map<unsigned /* label */, unsigned /* replacementLabel */> replacementLabels;

    for (auto& linkData : linked)
    {
        unsigned smallestLabel { std::numeric_limits<unsigned>::max() };

        for (auto& label : linkData.second.second)
            if (label < smallestLabel)
                smallestLabel = label;

        /*for (auto& label : linkData.second.second)
            replacementLabels[label] = smallestLabel;*/

        replacementLabels[linkData.first] = smallestLabel;
    }

    for (unsigned i {}; i < labels.size(); ++i)
    {
        auto it = replacementLabels.find(labels[i]);
        if (it != replacementLabels.end())
            labels[i] = it->second;
    }

    return labels;
}

std::unordered_map<unsigned, Nz::Vector2ui> getLabelNumber(const std::vector<unsigned>& labels, Nz::Vector2ui size)
{
    std::unordered_map<unsigned, Nz::Vector2ui> knownLabels;

    for (unsigned y {}; y < size.y; ++y)
        for (unsigned x {}; x < size.x; ++x)
        {
            unsigned label = labels[y * size.x + x];

            if (label != 0 && knownLabels.find(label) == knownLabels.end())
                knownLabels.insert(std::make_pair(label, Nz::Vector2ui { x, y }));
        }

    return knownLabels;
}

void visitLabel(const std::vector<unsigned>& labels, unsigned label, const Nz::Vector2ui& pos, const Nz::Vector2ui& size, Nz::Rectui& aabb, Nz::Bitset<>& visited)
{
    unsigned index = pos.y * size.x + pos.x;
    if (labels[index] == 0 /* != label*/ || visited.Test(index))
        return;

    visited.Set(index);

    // Re-set AABB
    {
        if (pos.x < aabb.x)
        {
            if (aabb.x != std::numeric_limits<unsigned>::max())
                aabb.width += (aabb.x - pos.x);
            aabb.x = pos.x;
        }

        if ((pos.x - aabb.x) > aabb.width)
            aabb.width = (pos.x - aabb.x);

        if (pos.y < aabb.y)
        {
            if (aabb.y != std::numeric_limits<unsigned>::max())
                aabb.height += (aabb.y - pos.y);
            aabb.y = pos.y;
        }

        if ((pos.y - aabb.y) > aabb.height)
            aabb.height = (pos.y - aabb.y);
    }

    // Hello neighbors
    {
        if ((pos.x + 1) < size.x)
            visitLabel(labels, label, Nz::Vector2ui { pos.x + 1, pos.y }, size, aabb, visited);
        if ((pos.y + 1) < size.y)
            visitLabel(labels, label, Nz::Vector2ui { pos.x, pos.y + 1 }, size, aabb, visited);

        if (pos.x > 0)
            visitLabel(labels, label, Nz::Vector2ui { pos.x - 1, pos.y }, size, aabb, visited);
        if (pos.y > 0)
            visitLabel(labels, label, Nz::Vector2ui { pos.x, pos.y - 1 }, size, aabb, visited);


        if (pos.x > 0 && pos.y > 0)
            visitLabel(labels, label, Nz::Vector2ui { pos.x - 1, pos.y - 1 }, size, aabb, visited);
        if (pos.x > 0 && (pos.y + 1) < size.y)
            visitLabel(labels, label, Nz::Vector2ui { pos.x - 1, pos.y + 1 }, size, aabb, visited);
        if ((pos.x + 1) < size.x && pos.y > 0)
            visitLabel(labels, label, Nz::Vector2ui { pos.x + 1, pos.y - 1 }, size, aabb, visited);
        if ((pos.x + 1) < size.x && (pos.y + 1) < size.y)
            visitLabel(labels, label, Nz::Vector2ui { pos.x + 1, pos.y + 1 }, size, aabb, visited);
    }
}
