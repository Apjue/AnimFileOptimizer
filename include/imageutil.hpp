// Copyright (C) 2019 Samy Bensaid
// This file is part of the AnimFileMaker project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TEAL_IMAGEUTIL_HPP
#define TEAL_IMAGEUTIL_HPP

#include <Nazara/Utility.hpp>
#include <Nazara/Core.hpp>
#include <array>
#include <vector>
#include <unordered_map>
#include <limits>
#include <set>

// https://en.wikipedia.org/wiki/Connected-component_labeling

extern std::vector<unsigned> getLabelsFromImage(const Nz::Image& img);
extern std::unordered_map<unsigned /* label */, Nz::Vector2ui /* first found */> getLabelNumber(const std::vector<unsigned>& labels, Nz::Vector2ui size);

extern void visitLabel(const std::vector<unsigned>& labels, unsigned label, const Nz::Vector2ui& pos, const Nz::Vector2ui& size, Nz::Rectui& aabb, Nz::Bitset<>& visited);

inline Nz::Rectui visitLabel(const std::vector<unsigned>& labels, unsigned label, const Nz::Vector2ui& pos, const Nz::Vector2ui& size)
{
    Nz::Rectui aabb { std::numeric_limits<unsigned>::max(), std::numeric_limits<unsigned>::max(), 0u, 0u };
    Nz::Bitset<> visited;
    visited.Resize(size.y * size.x);

    visitLabel(labels, label, pos, size, aabb, visited);
    return aabb;
}

#endif // TEAL_IMAGEUTIL_HPP
