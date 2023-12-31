/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

namespace AZ
{
    namespace Render 
    {
        namespace PencilMap 
        {
            // Please refer to "https://wiki.agscollab.com/display/ATOM/Pencil+Map" for details.

            // PencilMap 35mm Film
            static constexpr unsigned int TextureWidth = 128;
            static constexpr unsigned int TextureHeight = 64;
            static constexpr const char* TextureFilePath = "textures/postprocessing/depthoffield_pencilmap_35mmfilm.bmp.streamingimage";

            // EIS_35mm : 36.0mm x 24.0mm
            // sqrt(36.0 * 36.0 + 24.0 * 24.0) = 43.267mm = 0.043267m
            static constexpr float EIS_35mm_DiagonalLength = 0.043267f;

            // focusPointU(61) : Pixel number of focus point.
            // width(128) : The width of pencil map.
            // PencilMapFocusPointTexcoordU = (focusPointU + 1) / width.
            static constexpr float PencilMapFocusPointTexcoordU = 0.484375f;
        } // namespace PencilMap
    } // namespace Render
} // namespace AZ

