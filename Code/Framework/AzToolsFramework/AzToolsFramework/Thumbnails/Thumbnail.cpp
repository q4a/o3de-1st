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

#include <AzToolsFramework/Thumbnails/Thumbnail.h>
AZ_PUSH_DISABLE_WARNING(4127 4251 4800 4244, "-Wunknown-warning-option") // 4127: conditional expression is constant
                                                                         // 4251: 'QTextCodec::ConverterState::flags': class 'QFlags<QTextCodec::ConversionFlag>' needs to have dll-interface to be used by clients of struct 'QTextCodec::ConverterState'
                                                                         // 4800: 'QTextBoundaryFinderPrivate *const ': forcing value to bool 'true' or 'false' (performance warning)
                                                                         // 4244: conversion from 'int' to 'qint8', possible loss of data
#include <QtConcurrent/QtConcurrent>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        static const int DefaultIconSize = 16;

        //////////////////////////////////////////////////////////////////////////
        // ThumbnailKey
        //////////////////////////////////////////////////////////////////////////
        bool ThumbnailKey::IsReady() const { return m_ready; }

        bool ThumbnailKey::UpdateThumbnail()
        {
            if (!IsReady())
            {
                return false;
            }
            emit UpdateThumbnailSignal();
            return true;
        }

        //////////////////////////////////////////////////////////////////////////
        // Thumbnail
        //////////////////////////////////////////////////////////////////////////
        Thumbnail::Thumbnail(SharedThumbnailKey key, int thumbnailSize)
            : QObject()
            , m_state(State::Unloaded)
            , m_thumbnailSize(thumbnailSize)
            , m_key(key)
        {
            connect(&m_watcher, &QFutureWatcher<void>::finished, this, [this]()
                {
                    if (m_state == State::Loading)
                    {
                        m_state = State::Ready;
                    }
                    Q_EMIT Updated();
                });
        }

        Thumbnail::~Thumbnail() = default;

        bool Thumbnail::operator==(const Thumbnail& other) const
        {
            return m_key == other.m_key;
        }

        void Thumbnail::Load()
        {
            if (m_state == State::Unloaded)
            {
                m_state = State::Loading;
                QFuture<void> future = QtConcurrent::run([this](){ LoadThread(); });
                m_watcher.setFuture(future);
            }
        }

        QPixmap Thumbnail::GetPixmap() const
        {
            return GetPixmap(QSize(DefaultIconSize, DefaultIconSize));
        }

        QPixmap Thumbnail::GetPixmap(const QSize& size) const
        {
            if (m_icon.isNull())
            {
                return m_pixmap;
            }

            return m_icon.pixmap(size);
        }

        SharedThumbnailKey Thumbnail::GetKey() const
        {
            return m_key;
        }

        Thumbnail::State Thumbnail::GetState() const
        {
            return m_state;
        }

    } // namespace Thumbnailer
} // namespace AzToolsFramework

#include "Thumbnails/moc_Thumbnail.cpp"
