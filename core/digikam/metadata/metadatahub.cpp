/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2007-01-05
 * Description : Metadata handling
 *
 * Copyright (C) 2007-2011 by Marcel Wiesweg <marcel.wiesweg@gmx.de>
 * Copyright (C) 2007-2011 by Gilles Caulier <caulier dot gilles at gmail dot com>
 *
 * This program is free software you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation
 * either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#include "metadatahub.moc"

// Qt includes

#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>

// KDE includes

#include <kdebug.h>

// Libkexiv2 includes

#include <libkexiv2/version.h>

// Local includes

#include "databaseaccess.h"
#include "databasewatch.h"
#include "imageinfo.h"
#include "imagecomments.h"
#include "template.h"
#include "templatemanager.h"
#include "albumsettings.h"
#include "imageattributeswatch.h"
#include "tagscache.h"

namespace Digikam
{

class MetadataHub::MetadataHubPriv
{
public:

    MetadataHubPriv()
    {
        dateTimeStatus    = MetadataHub::MetadataInvalid;
        pickLabelStatus   = MetadataHub::MetadataInvalid;
        colorLabelStatus  = MetadataHub::MetadataInvalid;
        ratingStatus      = MetadataHub::MetadataInvalid;
        commentsStatus    = MetadataHub::MetadataInvalid;
        templateStatus    = MetadataHub::MetadataInvalid;

        pickLabel         = -1;
        colorLabel        = -1;
        highestPickLabel  = -1;
        highestColorLabel = -1;

        rating            = -1;
        highestRating     = -1;

        count             = 0;

        dateTimeChanged   = false;
        commentsChanged   = false;
        pickLabelChanged  = false;
        colorLabelChanged = false;
        ratingChanged     = false;
        templateChanged   = false;
        tagsChanged       = false;
    }

    bool                              dateTimeChanged;
    bool                              commentsChanged;
    bool                              pickLabelChanged;
    bool                              colorLabelChanged;
    bool                              ratingChanged;
    bool                              templateChanged;
    bool                              tagsChanged;

    int                               pickLabel;
    int                               highestPickLabel;
    int                               colorLabel;
    int                               highestColorLabel;
    int                               rating;
    int                               highestRating;
    int                               count;

    QDateTime                         dateTime;
    QDateTime                         lastDateTime;

    CaptionsMap                       comments;

    Template                          metadataTemplate;

    QMap<int, MetadataHub::TagStatus> tags;

    QStringList                       tagList;

    MetadataHub::Status               dateTimeStatus;
    MetadataHub::Status               commentsStatus;
    MetadataHub::Status               pickLabelStatus;
    MetadataHub::Status               colorLabelStatus;
    MetadataHub::Status               ratingStatus;
    MetadataHub::Status               templateStatus;

    template <class T> void loadWithInterval(const T& data, T& storage, T& highestStorage, MetadataHub::Status& status);
    template <class T> void loadSingleValue(const T& data, T& storage, MetadataHub::Status& status);
};

// ------------------------------------------------------------------------------------------

MetadataHub::MetadataHub()
    : d(new MetadataHubPriv)
{
}

MetadataHub::MetadataHub(const MetadataHub& other)
    : d(new MetadataHubPriv(*other.d))
{
}

MetadataHub::~MetadataHub()
{
    delete d;
}

MetadataHub& MetadataHub::operator=(const MetadataHub& other)
{
    (*d) = (*other.d);
    return *this;
}

void MetadataHub::reset()
{
    (*d) = MetadataHubPriv();
}

// --------------------------------------------------

void MetadataHub::load(const ImageInfo& info)
{
    d->count++;

    CaptionsMap commentMap;
    {
        DatabaseAccess access;
        ImageComments comments = info.imageComments(access);
        commentMap             = comments.toCaptionsMap();
    }

    Template tref = info.metadataTemplate();
    Template t    = TemplateManager::defaultManager()->findByContents(tref);
    //kDebug() << "Found Metadata Template: " << t.templateTitle();

    load(info.dateTime(),
         commentMap,
         info.colorLabel(),
         info.pickLabel(),
         info.rating(),
         t.isNull() ? tref : t);

    QList<int> tagIds = info.tagIds();
    loadTags(tagIds);
}

void MetadataHub::load(const DMetadata& metadata)
{
    d->count++;

    CaptionsMap comments;
    QStringList keywords;
    QDateTime   datetime;
    int         pickLabel;
    int         colorLabel;
    int         rating;

    // Try to get comments from image :
    // In first, from Xmp comments tag,
    // In second, from standard JPEG JFIF comments section,
    // In third, from Exif comments tag,
    // In four, from Iptc comments tag.
    comments = metadata.getImageComments();

    // Try to get date and time from image :
    // In first, from Exif date & time tags,
    // In second, from Xmp date & time tags, or
    // In third, from Iptc date & time tags.
    // else use file system time stamp.
    datetime = metadata.getImageDateTime();

    if ( !datetime.isValid() )
    {
        QFileInfo info( metadata.getFilePath() );
        datetime = info.lastModified();
    }

    // Try to get image pick label from Xmp tag
    pickLabel = metadata.getImagePickLabel();

    // Try to get image color label from Xmp tag
    colorLabel = metadata.getImageColorLabel();

    // Try to get image rating from Xmp tag, or Iptc Urgency tag
    rating = metadata.getImageRating();

    Template tref = metadata.getMetadataTemplate();
    Template t    = TemplateManager::defaultManager()->findByContents(tref);

    kDebug() << "Found Metadata Template: " << t.templateTitle();

    load(datetime, comments, colorLabel, pickLabel, rating, t.isNull() ? tref : t);

    // Try to get image tags from Xmp using digiKam namespace tags.

    QStringList tagPaths;

    if (metadata.getImageTagsPath(tagPaths))
    {
        QList<int> tagIds = TagsCache::instance()->tagsForPaths(tagPaths);
        loadTags(tagIds);
    }
}

bool MetadataHub::load(const QString& filePath, const MetadataSettingsContainer& settings)
{
    DMetadata metadata;
    metadata.setUseXMPSidecar4Reading(settings.useXMPSidecar4Reading);
    bool success = metadata.load(filePath);
    load(metadata); // increments count
    return success;
}

// private common code to merge tags
void MetadataHub::loadTags(const QList<int>& loadedTags)
{
    // get copy of tags
    QSet<int> previousTags = d->tags.keys().toSet();

    // first go through all tags contained in this set
    foreach (int tagId, loadedTags)
    {
        if (TagsCache::instance()->isInternalTag(tagId))
        {
            continue;
        }

        // that is a reference
        TagStatus& status = d->tags[tagId];

        // if it was not contained in the list, the default constructor will mark it as invalid
        if (status == MetadataInvalid)
        {
            if (d->count == 1)
                // there were no previous sets that could have contained the set
            {
                status = TagStatus(MetadataAvailable, true);
            }
            else
                // previous sets did not contain the tag, we do => disjoint
            {
                status = TagStatus(MetadataDisjoint, true);
            }
        }
        else if (status == TagStatus(MetadataAvailable, false))
        {
            // set to explicitly not contained, but we contain it => disjoint
            status = TagStatus(MetadataDisjoint, true);
        }

        // else if mapIt.value() ==  MetadataAvailable, true: all right, we contain it too
        // else if mapIt.value() ==  MetadataDisjoint: it's already disjoint

        // remove from the list to signal that this tag has been handled
        previousTags.remove(tagId);
    }

    // Those tags which had been set as MetadataAvailable before,
    // but are not contained in this set, have to be set to MetadataDisjoint
    foreach (int tagId, previousTags)
    {
        QMap<int, TagStatus>::iterator mapIt = d->tags.find(tagId);

        if (mapIt != d->tags.end() && mapIt.value() == TagStatus(MetadataAvailable, true))
        {
            mapIt.value() = TagStatus(MetadataDisjoint, true);
        }
    }
}

/*
// private code to merge tags with d->tagList
void MetadataHub::loadTags(const QStringList& loadedTagPaths)
{
    if (d->count == 1)
    {
        d->tagList = loadedTagPaths;
    }
    else
    {
        // a simple intersection
        QStringList toBeAdded;
        for (QStringList::iterator it = d->tagList.begin(); it != d->tagList.end(); ++it)
        {
            if (loadedTagPaths.indexOf(*it) == -1)
            {
                // it's not in the loadedTagPaths list. Remove it from intersection list.
                it = d->tagList.erase(it);
            }
            // else, it is in both lists, so no need to change d->tagList, it's already added.
        }
    }
}
*/

// private common code to load dateTime, comment, color label, pick label, rating
void MetadataHub::load(const QDateTime& dateTime, const CaptionsMap& comments,
                       int colorLabel, int pickLabel,
                       int rating, const Template& t)
{
    if (dateTime.isValid())
    {
        d->loadWithInterval<QDateTime>(dateTime, d->dateTime, d->lastDateTime, d->dateTimeStatus);
    }

    d->loadWithInterval<int>(pickLabel, d->pickLabel, d->highestPickLabel, d->pickLabelStatus);

    d->loadWithInterval<int>(colorLabel, d->colorLabel, d->highestColorLabel, d->colorLabelStatus);

    d->loadWithInterval<int>(rating, d->rating, d->highestRating, d->ratingStatus);

    d->loadSingleValue<CaptionsMap>(comments, d->comments, d->commentsStatus);

    d->loadSingleValue<Template>(t, d->metadataTemplate, d->templateStatus);
}

// template method to share code for dateTime, colorLabel, pickLabel, and rating
template <class T> void MetadataHub::MetadataHubPriv::loadWithInterval(const T& data,
        T& storage,
        T& highestStorage,
        MetadataHub::Status& status)
{
    switch (status)
    {
        case MetadataHub::MetadataInvalid:
            storage = data;
            status  = MetadataHub::MetadataAvailable;
            break;
        case MetadataHub::MetadataAvailable:

            // we have two values. If they are equal, status is unchanged
            if (data == storage)
            {
                break;
            }

            // they are not equal. We need to enter the disjoint state.
            status = MetadataHub::MetadataDisjoint;

            if (data > storage)
            {
                highestStorage = data;
            }
            else
            {
                highestStorage = storage;
                storage        = data;
            }

            break;
        case MetadataHub::MetadataDisjoint:

            // smaller value is stored in storage
            if (data < storage)
            {
                storage = data;
            }
            else if (highestStorage < data)
            {
                highestStorage = data;
            }

            break;
    }
}

// template method used by comment and template
template <class T> void MetadataHub::MetadataHubPriv::loadSingleValue(const T& data,
        T& storage,
        MetadataHub::Status& status)
{
    switch (status)
    {
        case MetadataHub::MetadataInvalid:
            storage = data;
            status  = MetadataHub::MetadataAvailable;
            break;
        case MetadataHub::MetadataAvailable:

            // we have two values. If they are equal, status is unchanged
            if (data == storage)
            {
                break;
            }

            // they are not equal. We need to enter the disjoint state.
            status = MetadataHub::MetadataDisjoint;
            break;
        case MetadataHub::MetadataDisjoint:
            break;
    }
}

// --------------------------------------------------

bool MetadataHub::write(ImageInfo info, WriteMode writeMode)
{
    applyChangeNotifications();

    bool changed = false;

    // find out in advance if we have something to write - needed for FullWriteIfChanged mode
    bool saveComment    = (d->commentsStatus   == MetadataAvailable);
    bool saveDateTime   = (d->dateTimeStatus   == MetadataAvailable);
    bool savePickLabel  = (d->pickLabelStatus  == MetadataAvailable);
    bool saveColorLabel = (d->colorLabelStatus == MetadataAvailable);
    bool saveRating     = (d->ratingStatus     == MetadataAvailable);
    bool saveTemplate   = (d->templateStatus   == MetadataAvailable);
    bool saveTags       = false;

    for (QMap<int, TagStatus>::iterator it = d->tags.begin(); it != d->tags.end(); ++it)
    {
        if (it.value() == MetadataAvailable)
        {
            saveTags = true;
            break;
        }
    }

    bool writeAllFields;

    if (writeMode == FullWrite)
    {
        writeAllFields = true;
    }
    else if (writeMode == FullWriteIfChanged)
        writeAllFields = (
                             (saveComment    && d->commentsChanged)   ||
                             (saveDateTime   && d->dateTimeChanged)   ||
                             (savePickLabel  && d->pickLabelChanged)  ||
                             (saveColorLabel && d->colorLabelChanged) ||
                             (saveRating     && d->ratingChanged)     ||
                             (saveTemplate   && d->templateChanged)   ||
                             (saveTags       && d->tagsChanged)
                         );
    else // PartialWrite
    {
        writeAllFields = false;
    }

    if (saveComment && (writeAllFields || d->commentsChanged))
    {
        DatabaseAccess access;
        ImageComments comments = info.imageComments(access);
        comments.replaceComments(d->comments);
        changed = true;
    }

    if (saveDateTime && (writeAllFields || d->dateTimeChanged))
    {
        info.setDateTime(d->dateTime);
        changed = true;
    }

    if (savePickLabel && (writeAllFields || d->pickLabelChanged))
    {
        info.setPickLabel(d->pickLabel);
        changed = true;
    }

    if (saveColorLabel && (writeAllFields || d->colorLabelChanged))
    {
        info.setColorLabel(d->colorLabel);
        changed = true;
    }

    if (saveRating && (writeAllFields || d->ratingChanged))
    {
        info.setRating(d->rating);
        changed = true;
    }

    if (saveTemplate && writeAllFields)
    {
        QString title = d->metadataTemplate.templateTitle();

        if (title == Template::removeTemplateTitle())
        {
            info.removeMetadataTemplate();
        }

        if (title.isEmpty())
        {
            // Nothing to do.
        }
        else
        {
            info.setMetadataTemplate(d->metadataTemplate);
        }

        changed = true;
    }

    if (writeAllFields || d->tagsChanged)
    {
        for (QMap<int, TagStatus>::iterator it = d->tags.begin(); it != d->tags.end(); ++it)
        {
            if (it.value() == MetadataAvailable)
            {
                if (it.value().hasTag)
                {
                    info.setTag(it.key());
                }
                else
                {
                    info.removeTag(it.key());
                }

                changed = true;
            }
        }
    }

    return changed;
}

bool MetadataHub::write(DMetadata& metadata, WriteMode writeMode, const MetadataSettingsContainer& settings)
{
    applyChangeNotifications();

    bool dirty = false;

    metadata.setWriteRawFiles(settings.writeRawFiles);
    metadata.setMetadataWritingMode(settings.metadataWritingMode);

#if KEXIV2_VERSION >= 0x000600
    metadata.setUpdateFileTimeStamp(settings.updateFileTimeStamp);
#endif

    // find out in advance if we have something to write - needed for FullWriteIfChanged mode
    bool saveComment    = (settings.saveComments   && d->commentsStatus   == MetadataAvailable);
    bool saveDateTime   = (settings.saveDateTime   && d->dateTimeStatus   == MetadataAvailable);
    bool savePickLabel  = (settings.savePickLabel  && d->pickLabelStatus  == MetadataAvailable);
    bool saveColorLabel = (settings.saveColorLabel && d->colorLabelStatus == MetadataAvailable);
    bool saveRating     = (settings.saveRating     && d->ratingStatus     == MetadataAvailable);
    bool saveTemplate   = (settings.saveTemplate   && d->templateStatus   == MetadataAvailable);
    bool saveTags       = false;

    if (settings.saveTags)
    {
        saveTags = false;

        // find at least one tag to write
        for (QMap<int, TagStatus>::iterator it = d->tags.begin(); it != d->tags.end(); ++it)
        {
            if (it.value() == MetadataAvailable)
            {
                saveTags = true;
                break;
            }
        }
    }

    bool writeAllFields;

    if (writeMode == FullWrite)
    {
        writeAllFields = true;
    }
    else if (writeMode == FullWriteIfChanged)
        writeAllFields = (
                             (saveComment    && d->commentsChanged)   ||
                             (saveDateTime   && d->dateTimeChanged)   ||
                             (savePickLabel  && d->pickLabelChanged)  ||
                             (saveColorLabel && d->colorLabelChanged) ||
                             (saveRating     && d->ratingChanged)     ||
                             (saveTemplate   && d->templateChanged)   ||
                             (saveTags       && d->tagsChanged)
                         );
    else // PartialWrite
    {
        writeAllFields = false;
    }

    if (saveComment && (writeAllFields || d->commentsChanged))
    {
        // Store comments in image as JFIF comments, Exif comments, Iptc Caption, and Xmp.
        dirty |= metadata.setImageComments(d->comments);
    }

    if (saveDateTime && (writeAllFields || d->dateTimeChanged))
    {
        // Store Image Date & Time as Exif and Iptc tags.
        dirty |= metadata.setImageDateTime(d->dateTime, false);
    }

    if (savePickLabel && (writeAllFields || d->pickLabelChanged))
    {
        // Store Image Pick Label as XMP tag.
        dirty |= metadata.setImagePickLabel(d->pickLabel);
    }

    if (saveColorLabel && (writeAllFields || d->colorLabelChanged))
    {
        // Store Image Color Label as XMP tag.
        dirty |= metadata.setImageColorLabel(d->colorLabel);
    }

    if (saveRating && (writeAllFields || d->ratingChanged))
    {
        // Store Image rating as Iptc tag.
        dirty |= metadata.setImageRating(d->rating);
    }

    if (saveTemplate && (writeAllFields || d->templateChanged))
    {
        QString title = d->metadataTemplate.templateTitle();

        if (title == Template::removeTemplateTitle())
        {
            dirty |= metadata.removeMetadataTemplate();
        }
        else if (title.isEmpty())
        {
            // Nothing to do.
        }
        else
        {
            // Store metadata template as XMP tag.
            dirty |= metadata.removeMetadataTemplate();
            dirty |= metadata.setMetadataTemplate(d->metadataTemplate);
        }
    }

    if (saveTags && (writeAllFields || d->tagsChanged))
    {
        // Store tag paths as Iptc keywords tags.

        // DatabaseMode == ManagedTags is assumed.
        // To fix this constraint (not needed currently), an oldKeywords parameter is needed

        // create list of keywords to be added and to be removed
        QStringList tagsPathList, oldKeywords, newKeywords;
        metadata.getImageTagsPath(tagsPathList);

        for (QMap<int, TagStatus>::iterator it = d->tags.begin(); it != d->tags.end(); ++it)
        {
            if (!TagsCache::instance()->canBeWrittenToMetadata(it.key()))
            {
                continue;
            }

            // it is important that MetadataDisjoint keywords are not touched
            if (it.value() == MetadataAvailable)
            {
                // This works for single and multiple selection.
                // In both situations, tags which had originally been loaded
                // have explicitly been removed with setTag.
                QString tagName = TagsCache::instance()->tagName(it.key());
                QString tagPath = TagsCache::instance()->tagPath(it.key(), TagsCache::NoLeadingSlash);
                if (it.value().hasTag)
                {
                    if (!tagsPathList.contains(tagPath))
                    {
                        tagsPathList << tagPath;
                    }
                    newKeywords << tagName;
                }
                else
                {
                    tagsPathList.removeAll(tagPath);
                    oldKeywords << tagName;
                }
            }
        }

        // NOTE: See B.K.O #175321 : we remove all old keyword from IPTC and XMP before to
        // synchronize metadata, else contents is not coherent.

        // We set Iptc keywords using tags name.
        dirty |= metadata.setIptcKeywords(oldKeywords, newKeywords);

        // We add Xmp keywords using tags name.
        dirty |= metadata.removeXmpKeywords(oldKeywords);
        dirty |= metadata.setXmpKeywords(newKeywords);

        // We set Tags Path list in digiKam Xmp private namespace using tags path.
        dirty |= metadata.setImageTagsPath(tagsPathList);
    }

    return dirty;
}

bool MetadataHub::write(const QString& filePath, WriteMode writeMode, const MetadataSettingsContainer& settings)
{
    applyChangeNotifications();

    // if no DMetadata object is needed at all, don't construct one -
    // important optimization if writing to file is turned off in setup!
    if (!willWriteMetadata(writeMode, settings))
    {
        return false;
    }

    DMetadata metadata(filePath);

    if (write(metadata, writeMode, settings))
    {
        bool success = metadata.applyChanges();
        ImageAttributesWatch::instance()->fileMetadataChanged(filePath);
        return success;
    }

    return false;
}

bool MetadataHub::write(DImg& image, WriteMode writeMode, const MetadataSettingsContainer& settings)
{
    applyChangeNotifications();

    // if no DMetadata object is needed at all, don't construct one
    if (!willWriteMetadata(writeMode, settings))
    {
        return false;
    }

    // See DImgLoader::readMetadata() and saveMetadata()
    DMetadata metadata;
    metadata.setData(image.getMetadata());

    return write(metadata, writeMode, settings);
}

bool MetadataHub::willWriteMetadata(WriteMode writeMode, const MetadataSettingsContainer& settings) const
{
    // This is the same logic as in write(DMetadata) but without actually writing.
    // Adapt if the method above changes

    bool saveComment    = (settings.saveComments   && d->commentsStatus   == MetadataAvailable);
    bool saveDateTime   = (settings.saveDateTime   && d->dateTimeStatus   == MetadataAvailable);
    bool savePickLabel  = (settings.savePickLabel  && d->pickLabelStatus  == MetadataAvailable);
    bool saveColorLabel = (settings.saveColorLabel && d->colorLabelStatus == MetadataAvailable);
    bool saveRating     = (settings.saveRating     && d->ratingStatus     == MetadataAvailable);
    bool saveTemplate   = (settings.saveTemplate   && d->templateStatus   == MetadataAvailable);
    bool saveTags       = false;

    if (settings.saveTags)
    {
        saveTags = false;

        // find at least one tag to write
        for (QMap<int, TagStatus>::iterator it = d->tags.begin(); it != d->tags.end(); ++it)
        {
            if (it.value() == MetadataAvailable)
            {
                saveTags = true;
                break;
            }
        }
    }

    bool writeAllFields;

    if (writeMode == FullWrite)
    {
        writeAllFields = true;
    }
    else if (writeMode == FullWriteIfChanged)
        writeAllFields = (
                             (saveComment    && d->commentsChanged)   ||
                             (saveDateTime   && d->dateTimeChanged)   ||
                             (savePickLabel  && d->pickLabelChanged)  ||
                             (saveColorLabel && d->colorLabelChanged) ||
                             (saveRating     && d->ratingChanged)     ||
                             (saveTemplate   && d->templateChanged)   ||
                             (saveTags       && d->tagsChanged)
                         );
    else // PartialWrite
    {
        writeAllFields = false;
    }

    return (
               (saveComment    && (writeAllFields || d->commentsChanged))   ||
               (saveDateTime   && (writeAllFields || d->dateTimeChanged))   ||
               (savePickLabel  && (writeAllFields || d->pickLabelChanged))  ||
               (saveColorLabel && (writeAllFields || d->colorLabelChanged)) ||
               (saveRating     && (writeAllFields || d->ratingChanged))     ||
               (saveTags       && (writeAllFields || d->tagsChanged))       ||
               (saveTemplate   && (writeAllFields || d->templateChanged))
           );
}

// --------------------------------------------------

MetadataHub::Status MetadataHub::dateTimeStatus() const
{
    return d->dateTimeStatus;
}

MetadataHub::Status MetadataHub::commentsStatus() const
{
    return d->commentsStatus;
}

MetadataHub::Status MetadataHub::pickLabelStatus() const
{
    return d->pickLabelStatus;
}

MetadataHub::Status MetadataHub::colorLabelStatus() const
{
    return d->colorLabelStatus;
}

MetadataHub::Status MetadataHub::ratingStatus() const
{
    return d->ratingStatus;
}

MetadataHub::Status MetadataHub::templateStatus() const
{
    return d->templateStatus;
}

MetadataHub::TagStatus MetadataHub::tagStatus(int tagId) const
{
    QMap<int, TagStatus>::iterator mapIt = d->tags.find(tagId);

    if (mapIt == d->tags.end())
    {
        return TagStatus(MetadataInvalid);
    }

    return mapIt.value();
}

MetadataHub::TagStatus MetadataHub::tagStatus(const QString& tagPath) const
{
    return tagStatus(TagsCache::instance()->tagForPath(tagPath));
}

bool MetadataHub::dateTimeChanged() const
{
    return d->dateTimeChanged;
}

bool MetadataHub::commentsChanged() const
{
    return d->commentsChanged;
}

bool MetadataHub::pickLabelChanged() const
{
    return d->pickLabelChanged;
}

bool MetadataHub::colorLabelChanged() const
{
    return d->colorLabelChanged;
}

bool MetadataHub::ratingChanged() const
{
    return d->ratingChanged;
}

bool MetadataHub::templateChanged() const
{
    return d->templateChanged;
}

bool MetadataHub::tagsChanged() const
{
    return d->tagsChanged;
}

QDateTime MetadataHub::dateTime() const
{
    return d->dateTime;
}

CaptionsMap MetadataHub::comments() const
{
    return d->comments;
}

int MetadataHub::pickLabel() const
{
    return d->pickLabel;
}

int MetadataHub::colorLabel() const
{
    return d->colorLabel;
}

int MetadataHub::rating() const
{
    return d->rating;
}

Template MetadataHub::metadataTemplate() const
{
    return d->metadataTemplate;
}

void MetadataHub::dateTimeInterval(QDateTime& lowest, QDateTime& highest) const
{
    switch (d->dateTimeStatus)
    {
        case MetadataInvalid:
            lowest = highest = QDateTime();
            break;
        case MetadataAvailable:
            lowest = highest = d->dateTime;
            break;
        case MetadataDisjoint:
            lowest  = d->dateTime;
            highest = d->lastDateTime;
            break;
    }
}

void MetadataHub::pickLabelInterval(int& lowest, int& highest) const
{
    switch (d->pickLabelStatus)
    {
        case MetadataInvalid:
            lowest = highest = -1;
            break;
        case MetadataAvailable:
            lowest = highest = d->pickLabel;
            break;
        case MetadataDisjoint:
            lowest  = d->pickLabel;
            highest = d->highestPickLabel;
            break;
    }
}

void MetadataHub::colorLabelInterval(int& lowest, int& highest) const
{
    switch (d->colorLabelStatus)
    {
        case MetadataInvalid:
            lowest = highest = -1;
            break;
        case MetadataAvailable:
            lowest = highest = d->colorLabel;
            break;
        case MetadataDisjoint:
            lowest  = d->colorLabel;
            highest = d->highestColorLabel;
            break;
    }
}

void MetadataHub::ratingInterval(int& lowest, int& highest) const
{
    switch (d->ratingStatus)
    {
        case MetadataInvalid:
            lowest = highest = -1;
            break;
        case MetadataAvailable:
            lowest = highest = d->rating;
            break;
        case MetadataDisjoint:
            lowest  = d->rating;
            highest = d->highestRating;
            break;
    }
}

QStringList MetadataHub::keywords() const
{
    QStringList tagList;

    for (QMap<int, TagStatus>::iterator it = d->tags.begin(); it != d->tags.end(); ++it)
    {
        if (it.value() == TagStatus(MetadataAvailable, true))
        {
            tagList.append(TagsCache::instance()->tagPath(it.key(), TagsCache::NoLeadingSlash));
        }
    }

    return tagList;
}

QMap<int, MetadataHub::TagStatus> MetadataHub::tags() const
{
    // DatabaseMode == ManagedTags is assumed
    return d->tags;
}

QMap<int, MetadataHub::TagStatus> MetadataHub::tagIDs() const
{
    // DatabaseMode == ManagedTags is assumed
    QMap<int, TagStatus> intmap;

    for (QMap<int, TagStatus>::const_iterator it = d->tags.constBegin(); it != d->tags.constEnd(); ++it)
    {
        intmap.insert(it.key(), it.value());
    }

    return intmap;
}

void MetadataHub::notifyTagDeleted(int tagId)
{
    d->tags.remove(tagId);
}

// --------------------------------------------------

void MetadataHub::setDateTime(const QDateTime& dateTime, Status status)
{
    d->dateTimeStatus  = status;
    d->dateTime        = dateTime;
    d->dateTimeChanged = true;
}

void MetadataHub::setComments(const CaptionsMap& comments, Status status)
{
    d->commentsStatus  = status;
    d->comments        = comments;
    d->commentsChanged = true;
}

void MetadataHub::setPickLabel(int pickId, Status status)
{
    d->pickLabelStatus  = status;
    d->pickLabel        = pickId;
    d->pickLabelChanged = true;
}

void MetadataHub::setColorLabel(int colorId, Status status)
{
    d->colorLabelStatus  = status;
    d->colorLabel        = colorId;
    d->colorLabelChanged = true;
}

void MetadataHub::setRating(int rating, Status status)
{
    d->ratingStatus   = status;
    d->rating         = rating;
    d->ratingChanged  = true;
}

void MetadataHub::setMetadataTemplate(const Template& t, Status status)
{
    d->templateStatus   = status;
    d->metadataTemplate = t;
    d->templateChanged  = true;
}

void MetadataHub::setTag(int tagId, bool hasTag, Status status)
{
    // DatabaseMode == ManagedTags is assumed
    d->tags[tagId] = TagStatus(status, hasTag);
    d->tagsChanged = true;
}

void MetadataHub::resetChanged()
{
    d->dateTimeChanged   = false;
    d->commentsChanged   = false;
    d->pickLabelChanged  = false;
    d->colorLabelChanged = false;
    d->ratingChanged     = false;
    d->templateChanged   = false;
    d->tagsChanged       = false;
}

void MetadataHub::applyChangeNotifications()
{
}

// --------------------------------------------------------------------------------------

class MetadataHubOnTheRoad::MetadataHubOnTheRoadPriv
{
public:

    MetadataHubOnTheRoadPriv()
    {
        invalid = false;
    }

    QMutex     mutex;
    QList<int> tagIds;
    bool       invalid;
};

MetadataHubOnTheRoad::MetadataHubOnTheRoad(QObject* parent)
    : QObject(parent), d(new MetadataHubOnTheRoadPriv)
{
    connect(TagsCache::instance(), SIGNAL(tagDeleted(int)),
            this, SLOT(slotTagDeleted(int)),
            Qt::DirectConnection);

    connect(DatabaseAccess::databaseWatch(), SIGNAL(databaseChanged()),
            this, SLOT(slotInvalidate()));
}

MetadataHubOnTheRoad::~MetadataHubOnTheRoad()
{
    delete d;
}

MetadataHubOnTheRoad& MetadataHubOnTheRoad::operator=(const MetadataHub& other)
{
    MetadataHub::operator=(other);
    return *this;
}

MetadataHubOnTheRoad::MetadataHubOnTheRoad(const MetadataHub& other)
    : QObject(0), MetadataHub(other), d(new MetadataHubOnTheRoadPriv)
{
}

MetadataHubOnTheRoad::MetadataHubOnTheRoad(const MetadataHubOnTheRoad& other, QObject* parent)
    : QObject(parent), MetadataHub(other), d(new MetadataHubOnTheRoadPriv)
{
    applyChangeNotifications();
}

void MetadataHubOnTheRoad::applyChangeNotifications()
{
    if (d->invalid)
    {
        reset();
    }

    QList<int> tagIds;
    {
        QMutexLocker locker(&d->mutex);
        tagIds = d->tagIds;
        d->tagIds.clear();
    }

    foreach (int tagId, tagIds)
    {
        notifyTagDeleted(tagId);
    }
}

void MetadataHubOnTheRoad::slotTagDeleted(int tagId)
{
    QMutexLocker locker(&d->mutex);
    d->tagIds << tagId;
}

void MetadataHubOnTheRoad::slotInvalidate()
{
    QMutexLocker locker(&d->mutex);
    d->invalid = true;
}

} // namespace Digikam
