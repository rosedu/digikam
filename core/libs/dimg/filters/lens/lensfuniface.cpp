/* ============================================================
 *
 * Date        : 2008-02-10
 * Description : a tool to fix automatically camera lens aberrations
 *
 * Copyright (C) 2008 by Adrian Schroeter <adrian at suse dot de>
 * Copyright (C) 2008-2010 by Gilles Caulier <caulier dot gilles at gmail dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#include "lensfuniface.h"

// KDE includes

#include <kdebug.h>

namespace Digikam
{

class LensFunIface::LensFunIfacePriv
{
public:

    LensFunIfacePriv()
    {
        usedLens   = 0;
        usedCamera = 0;
        lfDb       = 0;
        lfCameras  = 0;
    }

    // To be used for modification
    LensFunContainer       settings;

    // Database items
    lfDatabase*            lfDb;
    const lfCamera* const* lfCameras;

    QString                makeDescription;
    QString                modelDescription;
    QString                lensDescription;

    LensPtr                usedLens;
    DevicePtr              usedCamera;
};

LensFunIface::LensFunIface()
    : d(new LensFunIfacePriv)
{
    d->lfDb      = lf_db_new();
    d->lfDb->Load();
    d->lfCameras = d->lfDb->GetCameras();
}

LensFunIface::~LensFunIface()
{
    lf_db_destroy(d->lfDb);
    delete d;
}

void LensFunIface::setSettings(const LensFunContainer& other)
{
    d->settings   = other;
    d->usedCamera = findCamera(d->settings.cameraMake, d->settings.cameraModel);
    d->usedLens   = findLens(d->settings.lensModel);
}

LensFunContainer LensFunIface::settings() const
{
    return d->settings;
}

LensFunIface::DevicePtr LensFunIface::usedCamera() const
{
    return d->usedCamera;
}

void LensFunIface::setUsedCamera(DevicePtr cam)
{
    d->usedCamera           = cam;
    d->settings.cameraMake  = d->usedCamera ? d->usedCamera->Maker : QString();
    d->settings.cameraModel = d->usedCamera ? d->usedCamera->Model : QString();
}

LensFunIface::LensPtr LensFunIface::usedLens() const
{
    return d->usedLens;
}

void LensFunIface::setUsedLens(LensPtr lens)
{
    d->usedLens            = lens;
    d->settings.lensModel  = d->usedLens ? d->usedLens->Model : QString();
    d->settings.cropFactor = d->usedLens ? d->usedLens->CropFactor : -1.0;
}

lfDatabase* LensFunIface::lensFunDataBase() const
{
    return d->lfDb;
}

QString LensFunIface::makeDescription() const
{
    return d->makeDescription;
}

QString LensFunIface::modelDescription() const
{
    return d->modelDescription;
}

QString LensFunIface::lensDescription() const
{
    return d->lensDescription;
}

const lfCamera* const* LensFunIface::lensFunCameras() const
{
    return d->lfCameras;
}

void LensFunIface::setFilterSettings(const LensFunContainer& other)
{
    d->settings.filterCCA = other.filterCCA;
    d->settings.filterVIG = other.filterVIG;
    d->settings.filterCCI = other.filterCCI;
    d->settings.filterDST = other.filterDST;
    d->settings.filterGEO = other.filterGEO;
}

LensFunIface::DevicePtr LensFunIface::findCamera(const QString& make, const QString& model) const
{
    const lfCamera* const* lfCamera = d->lfDb->FindCameras( make.toAscii().constData(), model.toAscii().constData() );

    while (lfCamera && *lfCamera)
    {
        DevicePtr cam = *lfCamera;
        //        kDebug() << "Query camera:" << cam->Maker << "-" << cam->Model;

        if (QString(cam->Maker) == make &&
            QString(cam->Model) == model)
        {
            kDebug() << "Search for camera " << make << "-" << model << " ==> true";
            return cam;
        }

        ++lfCamera;
    }

    kDebug() << "Search for camera " << make << "-" << model << " ==> false";
    return 0;
}

LensFunIface::LensPtr LensFunIface::findLens(const QString& model) const
{
    const lfLens* const* lfLens = d->lfDb->GetLenses();

    while (lfLens && *lfLens)
    {
        LensPtr lens = *lfLens;

        if (QString(lens->Model) == model)
        {
            kDebug() << "Search for lens " << model << " ==> true";
            return lens;
        }

        ++lfLens;
    }

    kDebug() << "Search for lens " << model << " ==> false";
    return 0;
}

LensFunIface::LensList LensFunIface::findLenses(const lfCamera* lfCamera, const QString& lensDesc,
        const QString& lensMaker) const
{
    LensList lensList;
    const lfLens** lfLens = 0;

    if (lfCamera)
    {
        if (!lensMaker.isEmpty())
        {
            lfLens = d->lfDb->FindLenses(lfCamera, lensMaker.toAscii().constData(), lensDesc.toAscii().constData());
        }
        else
        {
            lfLens = d->lfDb->FindLenses(lfCamera, NULL, lensDesc.toAscii().constData());
        }

        while (lfLens && *lfLens)
        {
            lensList << (*lfLens);
            ++lfLens;
        }
    }

    return lensList;
}

LensFunIface::MetadataMatch LensFunIface::findFromMetadata(const DMetadata& meta)
{
    MetadataMatch ret  = MetadataNoMatch;
    d->settings        = LensFunContainer();
    d->usedCamera      = 0;
    d->usedLens        = 0;
    d->lensDescription = QString();

    if (meta.isEmpty())
    {
        kDebug() << "No metadata available";
        return LensFunIface::MetadataUnavailable;
    }

    PhotoInfoContainer photoInfo = meta.getPhotographInformation();
    d->makeDescription           = photoInfo.make.trimmed();
    d->modelDescription          = photoInfo.model.trimmed();
    bool exactMatch              = true;

    if (d->makeDescription.isEmpty())
    {
        kDebug() << "No camera maker info available";
        exactMatch = false;
    }
    else
    {
        // NOTE: see B.K.O #184156:
        // Some rules to wrap unkown camera device from Lensfun database, which have equivalent in fact.
        if (d->makeDescription == QString("Canon"))
        {
            if (d->modelDescription == QString("Canon EOS Kiss Digital X"))
            {
                d->modelDescription = QString("Canon EOS 400D DIGITAL");
            }
        }

        d->lensDescription = photoInfo.lens.trimmed();

        // ------------------------------------------------------------------------------------------------

        DevicePtr lfCamera = findCamera( d->makeDescription.toAscii().constData(), d->modelDescription.toAscii().constData() );

        if (lfCamera)
        {
            setUsedCamera(lfCamera);

            kDebug() << "Camera maker   : " << d->settings.cameraMake;
            kDebug() << "Camera model   : " << d->settings.cameraModel;

            // ------------------------------------------------------------------------------------------------
            // -- Performing lens description searches.

            if (!d->lensDescription.isEmpty())
            {

                QMap<int, LensPtr> bestMatches;
                QString            lensCutted;
                LensList           lensList;

                // STAGE 1, search in LensFun database as well.
                lensList = findLenses(d->usedCamera, d->lensDescription);
                kDebug() << "* Check for lens by direct query (" << d->lensDescription << " : " << lensList.count() << ")";

                if (!lensList.isEmpty())
                {
                    bestMatches.insert(lensList.count(), lensList[0]);
                }

                // STAGE 2, Adapt exiv2 strings to lensfun strings for Nikon.
                lensCutted = d->lensDescription;

                if (lensCutted.contains("Nikon"))
                {
                    lensCutted.replace("Nikon ", "");
                    lensCutted.replace("Zoom-", "");
                    lensCutted.replace("IF-ID", "ED-IF");
                    lensList = findLenses(d->usedCamera, lensCutted);
                    kDebug() << "* Check for Nikon lens (" << lensCutted << " : " << lensList.count() << ")";

                    if (!lensList.isEmpty())
                    {
                        bestMatches.insert(lensList.count(), lensList[0]);
                    }
                }

                // TODO : Add here more specific lens maker rules.

                // LAST STAGE, Adapt exiv2 strings to lensfun strings. Some lens description use something like that :
                // "10.0 - 20.0 mm". This must be adapted like this : "10-20mm"
                lensCutted = d->lensDescription;
                lensCutted.replace(QRegExp("\\.[0-9]"), "");
                lensCutted.replace(" - ", "-");
                lensCutted.replace(" mm", "mn");
                lensList = findLenses(d->usedCamera, lensCutted);
                kDebug() << "* Check for no maker lens (" << lensCutted << " : " << lensList.count() << ")";

                if (!lensList.isEmpty())
                {
                    bestMatches.insert(lensList.count(), lensList[0]);
                }

                // Display the results.

                if (bestMatches.isEmpty())
                {
                    kDebug() << "lens matches   : NOT FOUND";
                    exactMatch &= false;
                }
                else
                {
                    // Best case for an exact match is to have only one item returned by Lensfun searches.
                    QMap<int, LensPtr>::const_iterator it = bestMatches.constFind(1);

                    if (it != bestMatches.constEnd())
                    {
                        setUsedLens(bestMatches[it.key()]);
                        kDebug() << "Lens found     : " << d->settings.lensModel;
                        kDebug() << "Crop Factor    : " << d->settings.cropFactor;
                    }
                    else
                    {
                        kDebug() << "lens matches   : more than one...";
                        exactMatch &= false;
                    }
                }
            }
            else
            {
                kDebug() << "Lens description string is empty";
                exactMatch &= false;
            }
        }
        else
        {
            kDebug() << "Cannot find Lensfun camera device for (" << d->makeDescription << " - " << d->modelDescription << ")";
            exactMatch &= false;
        }
    }

    // ------------------------------------------------------------------------------------------------
    // Performing Lens settings searches.

    QString temp = photoInfo.focalLength;

    if (temp.isEmpty())
    {
        kDebug() << "Focal Length   : NOT FOUND";
        exactMatch &= false;
    }

    d->settings.focalLength = temp.mid(0, temp.length() -3).toDouble(); // HACK: strip the " mm" at the end ...
    kDebug() << "Focal Length   : " << d->settings.focalLength;

    // ------------------------------------------------------------------------------------------------

    temp = photoInfo.aperture;

    if (temp.isEmpty())
    {
        kDebug() << "Aperture       : NOT FOUND";
        exactMatch &= false;
    }

    d->settings.aperture = temp.mid(1).toDouble();
    kDebug() << "Aperture       : " << d->settings.aperture;

    // ------------------------------------------------------------------------------------------------
    // Try to get subject distance value.

    // From standard Exif.
    temp = meta.getExifTagString("Exif.Photo.SubjectDistance");

    if (temp.isEmpty())
    {
        // From standard XMP.
        temp = meta.getXmpTagString("Xmp.exif.SubjectDistance");
    }

    if (temp.isEmpty())
    {
        // From Canon Makernote.
        temp = meta.getExifTagString("Exif.CanonSi.SubjectDistance");
    }

    if (temp.isEmpty())
    {
        // From Nikon Makernote.
        temp = meta.getExifTagString("Exif.NikonLd2.FocusDistance");
    }

    if (temp.isEmpty())
    {
        // From Nikon Makernote.
        temp = meta.getExifTagString("Exif.NikonLd3.FocusDistance");
    }

    // TODO: Add here others Makernotes tags.

    if (temp.isEmpty())
    {
        kDebug() << "Subject dist.  : NOT FOUND";
        exactMatch &= false;
    }
    else
    {
        temp                        = temp.replace(" m", "");
        d->settings.subjectDistance = temp.toDouble();
        kDebug() << "Subject dist.  : " << d->settings.subjectDistance;
    }

    // ------------------------------------------------------------------------------------------------

    ret = exactMatch ? MetadataExactMatch : MetadataPartialMatch;

    kDebug() << "Metadata match : " << metadataMatchDebugStr(ret);

    return ret;
}

QString LensFunIface::metadataMatchDebugStr(MetadataMatch val) const
{
    QString ret;

    switch (val)
    {
        case MetadataNoMatch:
            ret = QString("No Match");
            break;
        case MetadataPartialMatch:
            ret = QString("Partial Match");
            break;
        default:
            ret = QString("Exact Match");
            break;
    }

    return ret;
}

bool LensFunIface::supportsDistortion() const
{
    if (!d->usedLens)
    {
        return false;
    }

    lfLensCalibDistortion res;
    return d->usedLens->InterpolateDistortion(d->settings.focalLength, res);
}

bool LensFunIface::supportsCCA() const
{
    if (!d->usedLens)
    {
        return false;
    }

    lfLensCalibTCA res;
    return d->usedLens->InterpolateTCA(d->settings.focalLength, res);
}

bool LensFunIface::supportsVig() const
{
    if (!d->usedLens)
    {
        return false;
    }

    lfLensCalibVignetting res;
    return d->usedLens->InterpolateVignetting(d->settings.focalLength,
            d->settings.aperture,
            d->settings.subjectDistance, res);
}

bool LensFunIface::supportsGeometry() const
{
    return supportsDistortion();
}

bool LensFunIface::supportsCCI() const
{
    return supportsVig();
}

}  // namespace Digikam
