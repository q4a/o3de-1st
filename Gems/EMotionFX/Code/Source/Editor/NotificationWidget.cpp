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
#include <Editor/NotificationWidget.h>
#include <MysticQt/Source/MysticQtManager.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QIcon>

namespace EMotionFX
{
    NotificationWidget::NotificationWidget(QWidget* parent, const QString& title)
        : QFrame(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

        QFrame* headerFrame = new QFrame(this);
        headerFrame->setObjectName("HeaderFrame");
        headerFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        headerFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
        headerFrame->setStyleSheet("background-color: rgb(60, 60, 60);");

        // Icon widget
        QLabel* iconLabel = new QLabel(headerFrame);
        iconLabel->setObjectName("Icon");
        QIcon icon(":/EMotionFX/Notification.svg");
        iconLabel->setPixmap(icon.pixmap(QSize(24, 24)));

        // Title widget
        QLabel* titleLabel = new QLabel(title, headerFrame);
        titleLabel->setObjectName("Title");
        titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        titleLabel->setWordWrap(true);

        QHBoxLayout* headerLayout = new QHBoxLayout(headerFrame);
        headerLayout->setSizeConstraint(QLayout::SetMinimumSize);
        headerLayout->setContentsMargins(QMargins(0, 0, 0, 0));
        headerLayout->addWidget(iconLabel);
        headerLayout->addWidget(titleLabel);
        headerFrame->setLayout(headerLayout);

        m_featureLayout = new QVBoxLayout(this);
        m_featureLayout->setSizeConstraint(QLayout::SetMinimumSize);
        m_featureLayout->setContentsMargins(QMargins(0, 0, 0, 0));
        m_featureLayout->addWidget(headerFrame);
    }

    void NotificationWidget::addFeature(QWidget* feature)
    {
        feature->setParent(this);
        m_featureLayout->addWidget(feature);
    }
}

