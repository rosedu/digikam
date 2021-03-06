/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2006-02-20
 * Description : a widget to display Standard Exif metadata
 *
 * Copyright (C) 2006-2010 by Gilles Caulier <caulier dot gilles at gmail dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#include "exifwidget.moc"

// Qt includes

#include <QMap>
#include <QFile>

// KDE includes


#include <klocale.h>

// Local includes

#include "dmetadata.h"
#include "metadatalistview.h"

namespace Digikam
{

// Standard Exif Entry list from to less important to the most important for photograph.
static const char* StandardExifEntryList[] =
{
    "Iop",
    "Thumbnail",
    "SubImage1",
    "SubImage2",
    "Image",
    "Photo",
    "GPSInfo",
    "-1"
};

ExifWidget::ExifWidget(QWidget* parent, const char* name)
    : MetadataWidget(parent, name)
{
    view()->setSortingEnabled(false);

    for (int i=0 ; QString(StandardExifEntryList[i]) != QString("-1") ; ++i)
    {
        m_keysFilter << StandardExifEntryList[i];
    }
}

ExifWidget::~ExifWidget()
{
}

QString ExifWidget::getMetadataTitle()
{
    return i18n("Standard EXIF Tags");
}

bool ExifWidget::loadFromURL(const KUrl& url)
{
    setFileName(url.toLocalFile());

    if (url.isEmpty())
    {
        setMetadata();
        return false;
    }
    else
    {
        DMetadata metadata(url.toLocalFile());

        if (!metadata.hasExif())
        {
            setMetadata();
            return false;
        }
        else
        {
            setMetadata(metadata);
        }
    }

    return true;
}

bool ExifWidget::decodeMetadata()
{
    DMetadata data = getMetadata();

    if (!data.hasExif())
    {
        return false;
    }

    // Update all metadata contents.
    setMetadataMap(data.getExifTagsDataList(m_keysFilter));
    return true;
}

void ExifWidget::buildView()
{
    if (getMode() == CUSTOM)
    {
        setIfdList(getMetadataMap(), m_keysFilter, getTagsFilter());
    }
    else
    {
        setIfdList(getMetadataMap(), m_keysFilter, QStringList() << QString("FULL"));
    }

    MetadataWidget::buildView();
}

QString ExifWidget::getTagTitle(const QString& key)
{
    DMetadata metadataIface;
    QString title = metadataIface.getExifTagTitle(key.toAscii());

    if (title.isEmpty())
    {
        return key.section('.', -1);
    }

    return title;
}

QString ExifWidget::getTagDescription(const QString& key)
{
    DMetadata metadataIface;
    QString desc = metadataIface.getExifTagDescription(key.toAscii());

    if (desc.isEmpty())
    {
        return i18n("No description available");
    }

    return desc;
}

void ExifWidget::slotSaveMetadataToFile()
{
    KUrl url = saveMetadataToFile(i18n("EXIF File to Save"),
                                  QString("*.exif|"+i18n("EXIF binary Files (*.exif)")));

#if KEXIV2_VERSION >= 0x010000
    storeMetadataToFile(url, getMetadata().getExifEncoded());
#else
    storeMetadataToFile(url, getMetadata().getExif());
#endif
}

}  // namespace Digikam
