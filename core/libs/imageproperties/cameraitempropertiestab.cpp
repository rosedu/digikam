/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2006-02-08
 * Description : A tab to display camera item information
 *
 * Copyright (C) 2006-2009 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#include "cameraitempropertiestab.moc"

// Qt includes

#include <QStyle>
#include <QFile>
#include <QGridLayout>

// KDE includes


#include <klocale.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kdialog.h>
#include <kfileitem.h>
#include <kmimetype.h>

// Local includes

#include "dmetadata.h"
#include "gpiteminfo.h"
#include "imagepropertiestxtlabel.h"

namespace Digikam
{

class CameraItemPropertiesTabPriv
{
public:

    CameraItemPropertiesTabPriv() :
        file(0),
        folder(0),
        date(0),
        size(0),
        isReadable(0),
        isWritable(0),
        mime(0),
        dimensions(0),
        newFileName(0),
        downloaded(0),
        make(0),
        model(0),
        photoDate(0),
        lens(0),
        aperture(0),
        focalLength(0),
        exposureTime(0),
        sensitivity(0),
        exposureMode(0),
        flash(0),
        whiteBalance(0),
        labelFile(0),
        labelFolder(0),
        labelFileIsReadable(0),
        labelFileIsWritable(0),
        labelFileDate(0),
        labelFileSize(0),
        labelImageMime(0),
        labelImageDimensions(0),
        labelNewFileName(0),
        labelAlreadyDownloaded(0),
        labelPhotoMake(0),
        labelPhotoModel(0),
        labelPhotoDateTime(0),
        labelPhotoLens(0),
        labelPhotoAperture(0),
        labelPhotoFocalLength(0),
        labelPhotoExposureTime(0),
        labelPhotoSensitivity(0),
        labelPhotoExposureMode(0),
        labelPhotoFlash(0),
        labelPhotoWhiteBalance(0)
    {
    }

    DTextLabelName*  file;
    DTextLabelName*  folder;
    DTextLabelName*  date;
    DTextLabelName*  size;
    DTextLabelName*  isReadable;
    DTextLabelName*  isWritable;
    DTextLabelName*  mime;
    DTextLabelName*  dimensions;
    DTextLabelName*  newFileName;
    DTextLabelName*  downloaded;

    DTextLabelName*  make;
    DTextLabelName*  model;
    DTextLabelName*  photoDate;
    DTextLabelName*  lens;
    DTextLabelName*  aperture;
    DTextLabelName*  focalLength;
    DTextLabelName*  exposureTime;
    DTextLabelName*  sensitivity;
    DTextLabelName*  exposureMode;
    DTextLabelName*  flash;
    DTextLabelName*  whiteBalance;

    DTextLabelValue* labelFile;
    DTextLabelValue* labelFolder;
    DTextLabelValue* labelFileIsReadable;
    DTextLabelValue* labelFileIsWritable;
    DTextLabelValue* labelFileDate;
    DTextLabelValue* labelFileSize;
    DTextLabelValue* labelImageMime;
    DTextLabelValue* labelImageDimensions;
    DTextLabelValue* labelNewFileName;
    DTextLabelValue* labelAlreadyDownloaded;

    DTextLabelValue* labelPhotoMake;
    DTextLabelValue* labelPhotoModel;
    DTextLabelValue* labelPhotoDateTime;
    DTextLabelValue* labelPhotoLens;
    DTextLabelValue* labelPhotoAperture;
    DTextLabelValue* labelPhotoFocalLength;
    DTextLabelValue* labelPhotoExposureTime;
    DTextLabelValue* labelPhotoSensitivity;
    DTextLabelValue* labelPhotoExposureMode;
    DTextLabelValue* labelPhotoFlash;
    DTextLabelValue* labelPhotoWhiteBalance;
};

CameraItemPropertiesTab::CameraItemPropertiesTab(QWidget* parent)
    : RExpanderBox(parent), d(new CameraItemPropertiesTabPriv)
{
    setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
    setLineWidth( style()->pixelMetric(QStyle::PM_DefaultFrameWidth) );

    // --------------------------------------------------

    QWidget* w1               = new QWidget(this);
    QGridLayout* glay1        = new QGridLayout(w1);

    d->file                   = new DTextLabelName(i18n("File: "),       w1);
    d->folder                 = new DTextLabelName(i18n("Folder: "),     w1);
    d->date                   = new DTextLabelName(i18n("Date: "),       w1);
    d->size                   = new DTextLabelName(i18n("Size: "),       w1);
    d->isReadable             = new DTextLabelName(i18n("Readable: "),   w1);
    d->isWritable             = new DTextLabelName(i18n("Writable: "),   w1);
    d->mime                   = new DTextLabelName(i18n("Type: "),       w1);
    d->dimensions             = new DTextLabelName(i18n("Dimensions: "), w1);
    d->newFileName            = new DTextLabelName(i18n("New Name: "),   w1);
    d->downloaded             = new DTextLabelName(i18n("Downloaded: "), w1);

    d->labelFile              = new DTextLabelValue(0, w1);
    d->labelFolder            = new DTextLabelValue(0, w1);
    d->labelFileDate          = new DTextLabelValue(0, w1);
    d->labelFileSize          = new DTextLabelValue(0, w1);
    d->labelFileIsReadable    = new DTextLabelValue(0, w1);
    d->labelFileIsWritable    = new DTextLabelValue(0, w1);
    d->labelImageMime         = new DTextLabelValue(0, w1);
    d->labelImageDimensions   = new DTextLabelValue(0, w1);
    d->labelNewFileName       = new DTextLabelValue(0, w1);
    d->labelAlreadyDownloaded = new DTextLabelValue(0, w1);

    glay1->addWidget(d->file,                   0, 0, 1, 1);
    glay1->addWidget(d->labelFile,              0, 1, 1, 1);
    glay1->addWidget(d->folder,                 1, 0, 1, 1);
    glay1->addWidget(d->labelFolder,            1, 1, 1, 1);
    glay1->addWidget(d->date,                   2, 0, 1, 1);
    glay1->addWidget(d->labelFileDate,          2, 1, 1, 1);
    glay1->addWidget(d->size,                   3, 0, 1, 1);
    glay1->addWidget(d->labelFileSize,          3, 1, 1, 1);
    glay1->addWidget(d->isReadable,             4, 0, 1, 1);
    glay1->addWidget(d->labelFileIsReadable,    4, 1, 1, 1);
    glay1->addWidget(d->isWritable,             5, 0, 1, 1);
    glay1->addWidget(d->labelFileIsWritable,    5, 1, 1, 1);
    glay1->addWidget(d->mime,                   6, 0, 1, 1);
    glay1->addWidget(d->labelImageMime,         6, 1, 1, 1);
    glay1->addWidget(d->dimensions,             7, 0, 1, 1);
    glay1->addWidget(d->labelImageDimensions,   7, 1, 1, 1);
    glay1->addWidget(d->newFileName,            8, 0, 1, 1);
    glay1->addWidget(d->labelNewFileName,       8, 1, 1, 1);
    glay1->addWidget(d->downloaded,             9, 0, 1, 1);
    glay1->addWidget(d->labelAlreadyDownloaded, 9, 1, 1, 1);
    glay1->setColumnStretch(1, 10);
    glay1->setMargin(KDialog::spacingHint());
    glay1->setSpacing(0);

    addItem(w1, SmallIcon("dialog-information"),
            i18n("Camera File Properties"), QString("FileProperties"), true);

    // --------------------------------------------------

    QWidget* w2               = new QWidget(this);
    QGridLayout* glay2        = new QGridLayout(w2);

    d->make                   = new DTextLabelName(i18n("Make: "),          w2);
    d->model                  = new DTextLabelName(i18n("Model: "),         w2);
    d->photoDate              = new DTextLabelName(i18n("Created: "),       w2);
    d->lens                   = new DTextLabelName(i18n("Lens: "),          w2);
    d->aperture               = new DTextLabelName(i18n("Aperture: "),      w2);
    d->focalLength            = new DTextLabelName(i18n("Focal: "),         w2);
    d->exposureTime           = new DTextLabelName(i18n("Exposure: "),      w2);
    d->sensitivity            = new DTextLabelName(i18n("Sensitivity: "),   w2);
    d->exposureMode           = new DTextLabelName(i18n("Mode/Program: "),  w2);
    d->flash                  = new DTextLabelName(i18n("Flash: "),         w2);
    d->whiteBalance           = new DTextLabelName(i18n("White balance: "), w2);

    d->labelPhotoMake         = new DTextLabelValue(0, w2);
    d->labelPhotoModel        = new DTextLabelValue(0, w2);
    d->labelPhotoDateTime     = new DTextLabelValue(0, w2);
    d->labelPhotoLens         = new DTextLabelValue(0, w2);
    d->labelPhotoAperture     = new DTextLabelValue(0, w2);
    d->labelPhotoFocalLength  = new DTextLabelValue(0, w2);
    d->labelPhotoExposureTime = new DTextLabelValue(0, w2);
    d->labelPhotoSensitivity  = new DTextLabelValue(0, w2);
    d->labelPhotoExposureMode = new DTextLabelValue(0, w2);
    d->labelPhotoFlash        = new DTextLabelValue(0, w2);
    d->labelPhotoWhiteBalance = new DTextLabelValue(0, w2);

    glay2->addWidget(d->make,                    0, 0, 1, 1);
    glay2->addWidget(d->labelPhotoMake,          0, 1, 1, 1);
    glay2->addWidget(d->model,                   1, 0, 1, 1);
    glay2->addWidget(d->labelPhotoModel,         1, 1, 1, 1);
    glay2->addWidget(d->photoDate,               2, 0, 1, 1);
    glay2->addWidget(d->labelPhotoDateTime,      2, 1, 1, 1);
    glay2->addWidget(d->lens,                    3, 0, 1, 1);
    glay2->addWidget(d->labelPhotoLens,          3, 1, 1, 1);
    glay2->addWidget(d->aperture,                4, 0, 1, 1);
    glay2->addWidget(d->labelPhotoAperture,      4, 1, 1, 1);
    glay2->addWidget(d->focalLength,             5, 0, 1, 1);
    glay2->addWidget(d->labelPhotoFocalLength,   5, 1, 1, 1);
    glay2->addWidget(d->exposureTime,            6, 0, 1, 1);
    glay2->addWidget(d->labelPhotoExposureTime,  6, 1, 1, 1);
    glay2->addWidget(d->sensitivity,             7, 0, 1, 1);
    glay2->addWidget(d->labelPhotoSensitivity,   7, 1, 1, 1);
    glay2->addWidget(d->exposureMode,            8, 0, 1, 1);
    glay2->addWidget(d->labelPhotoExposureMode,  8, 1, 1, 1);
    glay2->addWidget(d->flash,                   9, 0, 1, 1);
    glay2->addWidget(d->labelPhotoFlash,         9, 1, 1, 1);
    glay2->addWidget(d->whiteBalance,           10, 0, 1, 1);
    glay2->addWidget(d->labelPhotoWhiteBalance, 10, 1, 1, 1);
    glay2->setColumnStretch(1, 10);
    glay2->setMargin(KDialog::spacingHint());
    glay2->setSpacing(0);

    addItem(w2, SmallIcon("camera-photo"),
            i18n("Photograph Properties"), QString("PhotographProperties"), true);

    addStretch();
}

CameraItemPropertiesTab::~CameraItemPropertiesTab()
{
    delete d;
}

void CameraItemPropertiesTab::setCurrentItem(const GPItemInfo* itemInfo,
        const QString& newFileName, const QByteArray& exifData,
        const KUrl& currentURL)
{
    if (!itemInfo)
    {
        d->labelFile->setText(QString());
        d->labelFolder->setText(QString());
        d->labelFileIsReadable->setText(QString());
        d->labelFileIsWritable->setText(QString());
        d->labelFileDate->setText(QString());
        d->labelFileSize->setText(QString());
        d->labelImageMime->setText(QString());
        d->labelImageDimensions->setText(QString());
        d->labelNewFileName->setText(QString());
        d->labelAlreadyDownloaded->setText(QString());

        d->labelPhotoMake->setText(QString());
        d->labelPhotoModel->setText(QString());
        d->labelPhotoDateTime->setText(QString());
        d->labelPhotoLens->setText(QString());
        d->labelPhotoAperture->setText(QString());
        d->labelPhotoFocalLength->setText(QString());
        d->labelPhotoExposureTime->setText(QString());
        d->labelPhotoSensitivity->setText(QString());
        d->labelPhotoExposureMode->setText(QString());
        d->labelPhotoFlash->setText(QString());
        d->labelPhotoWhiteBalance->setText(QString());

        setEnabled(false);
        return;
    }

    setEnabled(true);

    QString str;
    QString unknown(i18n("<i>unknown</i>"));

    // -- Camera file system information ------------------------------------------

    d->labelFile->setText(itemInfo->name);
    d->labelFolder->setText(itemInfo->folder);

    if (itemInfo->readPermissions < 0)
    {
        str = unknown;
    }
    else if (itemInfo->readPermissions == 0)
    {
        str = i18n("No");
    }
    else
    {
        str = i18n("Yes");
    }

    d->labelFileIsReadable->setText(str);

    if (itemInfo->writePermissions < 0)
    {
        str = unknown;
    }
    else if (itemInfo->writePermissions == 0)
    {
        str = i18n("No");
    }
    else
    {
        str = i18n("Yes");
    }

    d->labelFileIsWritable->setText(str);

    if (itemInfo->mtime.isValid())
        d->labelFileDate->setText(KGlobal::locale()->formatDateTime(itemInfo->mtime,
                                  KLocale::ShortDate, true));
    else
    {
        d->labelFileDate->setText(unknown);
    }

    str = i18n("%1 (%2)", KIO::convertSize(itemInfo->size),
               KGlobal::locale()->formatNumber(itemInfo->size, 0));
    d->labelFileSize->setText(str);

    // -- Image Properties --------------------------------------------------

    if (itemInfo->mime == "image/x-raw")
    {
        d->labelImageMime->setText(i18n("RAW Image"));
    }
    else
    {
        KMimeType::Ptr mimeType = KMimeType::mimeType(itemInfo->mime, KMimeType::ResolveAliases);

        if (mimeType)
        {
            d->labelImageMime->setText(mimeType->comment());
        }
        else
        {
            d->labelImageMime->setText(itemInfo->mime);    // last fallback
        }
    }

    QString mpixels;
    QSize dims;

    if (itemInfo->width == -1 && itemInfo->height == -1 && !currentURL.isEmpty())
    {
        // delayed loading to list faster from UMSCamera
        if (itemInfo->mime == "image/x-raw")
        {
            DMetadata metaData(currentURL.toLocalFile());
            dims = metaData.getImageDimensions();
        }
        else
        {
            KFileMetaInfo meta(currentURL.toLocalFile());

            /*          TODO: KDE4PORT: KFileMetaInfo API as Changed.
                                        Check if new method to search "Dimensions" information is enough.

                        if (meta.isValid())
                        {
                            if (meta.containsGroup("Jpeg EXIF Data"))
                                dims = meta.group("Jpeg EXIF Data").item("Dimensions").value().toSize();
                            else if (meta.containsGroup("General"))
                                dims = meta.group("General").item("Dimensions").value().toSize();
                            else if (meta.containsGroup("Technical"))
                                dims = meta.group("Technical").item("Dimensions").value().toSize();
                        }*/

            if (meta.isValid() && meta.item("Dimensions").isValid())
            {
                dims = meta.item("Dimensions").value().toSize();
            }
        }
    }
    else
    {
        // if available (GPCamera), take dimensions directly from itemInfo
        dims = QSize(itemInfo->width, itemInfo->height);
    }

    mpixels.setNum(dims.width()*dims.height()/1000000.0, 'f', 2);
    str = (!dims.isValid()) ? unknown : i18n("%1x%2 (%3Mpx)",
            dims.width(), dims.height(), mpixels);
    d->labelImageDimensions->setText(str);

    // -- Download information ------------------------------------------

    d->labelNewFileName->setText(newFileName.isEmpty() ? i18n("<i>unchanged</i>") : newFileName);

    if (itemInfo->downloaded == GPItemInfo::DownloadUnknown)
    {
        str = unknown;
    }
    else if (itemInfo->downloaded == GPItemInfo::DownloadedYes)
    {
        str = i18n("Yes");
    }
    else
    {
        str = i18n("No");
    }

    d->labelAlreadyDownloaded->setText(str);

    // -- Photograph information ------------------------------------------
    // Note: If something is changed here, please updated albumfiletip section too.

    QString unavailable(i18n("<i>unavailable</i>"));
    DMetadata metaData;
    metaData.setExif(exifData);
    PhotoInfoContainer photoInfo = metaData.getPhotographInformation();

    if (photoInfo.isEmpty())
    {
        widget(1)->hide();
    }
    else
    {
        widget(1)->show();
    }

    d->labelPhotoMake->setText(photoInfo.make.isEmpty() ? unavailable : photoInfo.make);
    d->labelPhotoModel->setText(photoInfo.model.isEmpty() ? unavailable : photoInfo.model);

    if (photoInfo.dateTime.isValid())
    {
        str = KGlobal::locale()->formatDateTime(photoInfo.dateTime, KLocale::ShortDate, true);
        d->labelPhotoDateTime->setText(str);
    }
    else
    {
        d->labelPhotoDateTime->setText(unavailable);
    }

    d->labelPhotoLens->setText(photoInfo.lens.isEmpty() ? unavailable : photoInfo.lens);
    d->labelPhotoAperture->setText(photoInfo.aperture.isEmpty() ? unavailable : photoInfo.aperture);

    if (photoInfo.focalLength35mm.isEmpty())
    {
        d->labelPhotoFocalLength->setText(photoInfo.focalLength.isEmpty() ? unavailable : photoInfo.focalLength);
    }
    else
    {
        str = i18n("%1 (35mm: %2)", photoInfo.focalLength,
                   photoInfo.focalLength35mm);
        d->labelPhotoFocalLength->setText(str);
    }

    d->labelPhotoExposureTime->setText(photoInfo.exposureTime.isEmpty() ? unavailable : photoInfo.exposureTime);
    d->labelPhotoSensitivity->setText(photoInfo.sensitivity.isEmpty() ? unavailable : i18n("%1 ISO", photoInfo.sensitivity));

    if (photoInfo.exposureMode.isEmpty() && photoInfo.exposureProgram.isEmpty())
    {
        d->labelPhotoExposureMode->setText(unavailable);
    }
    else if (!photoInfo.exposureMode.isEmpty() && photoInfo.exposureProgram.isEmpty())
    {
        d->labelPhotoExposureMode->setText(photoInfo.exposureMode);
    }
    else if (photoInfo.exposureMode.isEmpty() && !photoInfo.exposureProgram.isEmpty())
    {
        d->labelPhotoExposureMode->setText(photoInfo.exposureProgram);
    }
    else
    {
        str = QString("%1 / %2").arg(photoInfo.exposureMode).arg(photoInfo.exposureProgram);
        d->labelPhotoExposureMode->setText(str);
    }

    d->labelPhotoFlash->setText(photoInfo.flash.isEmpty() ? unavailable : photoInfo.flash);
    d->labelPhotoWhiteBalance->setText(photoInfo.whiteBalance.isEmpty() ? unavailable : photoInfo.whiteBalance);
}

}  // namespace Digikam
