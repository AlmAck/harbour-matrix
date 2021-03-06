/**************************************************************************
 *                                                                        *
 * Copyright (C) 2016 Felix Rohrbach <kde@fxrh.de>                        *
 *                                                                        *
 * This program is free software; you can redistribute it and/or          *
 * modify it under the terms of the GNU General Public License            *
 * as published by the Free Software Foundation; either version 3         *
 * of the License, or (at your option) any later version.                 *
 *                                                                        *
 * This program is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 *                                                                        *
 **************************************************************************/

#include "imageprovider.h"

#include "lib/connection.h"
#include "jobs/mediathumbnailjob.h"

#include <QtCore/QDebug>

ImageProvider::ImageProvider(QMatrixClient::Connection* connection)
    : QQuickImageProvider(QQmlImageProviderBase::Pixmap, QQmlImageProviderBase::ForceAsynchronousImageLoading),
      m_connection(connection)
{
    qRegisterMetaType<QPixmap*>();
    qRegisterMetaType<QWaitCondition*>();
}

QPixmap ImageProvider::requestPixmap(const QString& id,
                                     QSize* size, const QSize& requestedSize)
{
    QPixmap result;

    if(id == "")
    {
        return result;
    }

    QWaitCondition condition;

    QMetaObject::invokeMethod(this, "doRequest", Qt::QueuedConnection,
                              Q_ARG(QString, "mxc://" + id), Q_ARG(QSize, requestedSize),
                              Q_ARG(QPixmap*, &result),
                              Q_ARG(QWaitCondition*, &condition));
    condition.wait(&m_mutex);

    if( size != nullptr )
    {
        *size = result.size();
    }

    return result;
}

void ImageProvider::setConnection(const QMatrixClient::Connection* connection)
{
    m_connection = connection;
}

void ImageProvider::doRequest(QString id, QSize requestedSize, QPixmap* pixmap,
                              QWaitCondition* condition)
{
    Q_ASSERT(pixmap);
    Q_ASSERT(condition);
    if( !m_connection )
    {
        qDebug() << "ImageProvider::requestPixmap: no connection!";
        *pixmap = QPixmap();
        condition->wakeAll();
        return;
    }

    using QMatrixClient::MediaThumbnailJob;
    auto job = m_connection->callApi<MediaThumbnailJob>(QUrl(id), requestedSize);

    connect( job, &MediaThumbnailJob::finished, this, [=]()
    {
        // TODO: need to check result to see if this is success or not
        // No need to lock because we don't deal with the ImageProvider state
        *pixmap = job->thumbnail();
        condition->wakeAll();
    } );
}

