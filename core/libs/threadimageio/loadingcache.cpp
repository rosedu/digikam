/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2006-01-11
 * Description : shared image loading and caching
 *
 * Copyright (C) 2005-2008 by Marcel Wiesweg <marcel.wiesweg@gmx.de>
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

#include "loadingcache.moc"

// Qt includes

#include <QCoreApplication>
#include <QEvent>
#include <QCustomEvent>
#include <QCache>
#include <QHash>

// KDE includes

#include <kdirwatch.h>
#include <kdebug.h>

// Local includes

#include "iccsettings.h"
#include "kmemoryinfo.h"

namespace Digikam
{

class LoadingCachePriv
{
public:

    LoadingCachePriv(LoadingCache* q) : q(q)
    {
        // Note: Don't make the mutex recursive, we need to use a wait condition on it
        watch = 0;
    }

    QCache<QString, DImg>           imageCache;
    QCache<QString, QImage>         thumbnailImageCache;
    QCache<QString, QPixmap>        thumbnailPixmapCache;
    QMultiHash<QString, QString>    imageFilePathHash;
    QMultiHash<QString, QString>    thumbnailFilePathHash;
    QHash<QString, LoadingProcess*> loadingDict;
    QMutex                          mutex;
    QWaitCondition                  condVar;
    LoadingCacheFileWatch*          watch;

    void mapImageFilePath(const QString& filePath, const QString& cacheKey);
    void mapThumbnailFilePath(const QString& filePath, const QString& cacheKey);
    void cleanUpImageFilePathHash();
    void cleanUpThumbnailFilePathHash();
    LoadingCacheFileWatch* fileWatch();

    LoadingCache* q;
};

LoadingCache* LoadingCache::m_instance = 0;

LoadingCache* LoadingCache::cache()
{
    if (!m_instance)
    {
        m_instance = new LoadingCache;
    }

    return m_instance;
}

void LoadingCache::cleanUp()
{
    delete m_instance;
}

LoadingCache::LoadingCache()
    : d(new LoadingCachePriv(this))
{
    KMemoryInfo memory = KMemoryInfo::currentInfo();
    setCacheSize(qBound(60, int(memory.megabytes(KMemoryInfo::TotalRam)*0.05), 200));
    setThumbnailCacheSize(5, 100); // the pixmap number should not be based on system memory, it's graphics memory

    // good place to call it here as LoadingCache is a singleton
    qRegisterMetaType<LoadingDescription>("LoadingDescription");
    qRegisterMetaType<DImg>("DImg");

    connect(IccSettings::instance(), SIGNAL(settingsChanged(const ICCSettingsContainer&, const ICCSettingsContainer&)),
            this, SLOT(iccSettingsChanged(const ICCSettingsContainer&, const ICCSettingsContainer&)));
}

LoadingCache::~LoadingCache()
{
    delete d->watch;
    delete d;
    m_instance = 0;
}

DImg* LoadingCache::retrieveImage(const QString& cacheKey)
{
    return d->imageCache[cacheKey];
}

bool LoadingCache::putImage(const QString& cacheKey, DImg* img, const QString& filePath)
{
    bool successfulyInserted;

    int cost = img->numBytes();

    successfulyInserted = d->imageCache.insert(cacheKey, img, cost);

    if (successfulyInserted && !filePath.isEmpty())
    {
        d->mapImageFilePath(filePath, cacheKey);
        d->fileWatch()->addedImage(filePath);
    }

    return successfulyInserted;
}

void LoadingCache::removeImage(const QString& cacheKey)
{
    d->imageCache.remove(cacheKey);
}

void LoadingCache::removeImages()
{
    d->imageCache.clear();
}

bool LoadingCache::isCacheable(const DImg* img)
{
    // return whether image fits in cache
    return (uint)d->imageCache.maxCost() >= img->numBytes();
}

void LoadingCache::addLoadingProcess(LoadingProcess* process)
{
    d->loadingDict[process->cacheKey()] = process;
}

LoadingProcess* LoadingCache::retrieveLoadingProcess(const QString& cacheKey)
{
    return d->loadingDict.value(cacheKey);
}

void LoadingCache::removeLoadingProcess(LoadingProcess* process)
{
    d->loadingDict.remove(process->cacheKey());
}

void LoadingCache::notifyNewLoadingProcess(LoadingProcess* process, LoadingDescription description)
{
    for (QHash<QString, LoadingProcess*>::const_iterator it = d->loadingDict.constBegin();
         it != d->loadingDict.constEnd(); ++it)
    {
        it.value()->notifyNewLoadingProcess(process, description);
    }
}

void LoadingCache::setCacheSize(int megabytes)
{
    kDebug() << "Allowing a cache size of" << megabytes << "MB";
    d->imageCache.setMaxCost(megabytes * 1024 * 1024);
}

// --- Thumbnails ----

const QImage* LoadingCache::retrieveThumbnail(const QString& cacheKey) const
{
    return d->thumbnailImageCache[cacheKey];
}

const QPixmap* LoadingCache::retrieveThumbnailPixmap(const QString& cacheKey) const
{
    return d->thumbnailPixmapCache[cacheKey];
}

bool LoadingCache::hasThumbnailPixmap(const QString& cacheKey) const
{
    return d->thumbnailPixmapCache.contains(cacheKey);
}

void LoadingCache::putThumbnail(const QString& cacheKey, const QImage& thumb, const QString& filePath)
{
    int cost = thumb.numBytes();

    if (d->thumbnailImageCache.insert(cacheKey, new QImage(thumb), cost))
    {
        d->mapThumbnailFilePath(filePath, cacheKey);
        d->fileWatch()->addedThumbnail(filePath);
    }

}

void LoadingCache::putThumbnail(const QString& cacheKey, const QPixmap& thumb, const QString& filePath)
{
    int cost = thumb.width() * thumb.height() * thumb.depth() / 8;

    if (d->thumbnailPixmapCache.insert(cacheKey, new QPixmap(thumb), cost))
    {
        d->mapThumbnailFilePath(filePath, cacheKey);
        d->fileWatch()->addedThumbnail(filePath);
    }
}

void LoadingCache::removeThumbnail(const QString& cacheKey)
{
    d->thumbnailImageCache.remove(cacheKey);
    d->thumbnailPixmapCache.remove(cacheKey);
}

void LoadingCache::removeThumbnails()
{
    d->thumbnailImageCache.clear();
    d->thumbnailPixmapCache.clear();
}

void LoadingCache::setThumbnailCacheSize(int numberOfQImages, int numberOfQPixmaps)
{
    d->thumbnailImageCache.setMaxCost(numberOfQImages * 256 * 256 * 4);
    d->thumbnailPixmapCache.setMaxCost(numberOfQPixmaps * 256 * 256 * QPixmap::defaultDepth() / 8);
}

void LoadingCache::setFileWatch(LoadingCacheFileWatch* watch)
{
    delete d->watch;
    d->watch = watch;
    d->watch->m_cache = this;
}

QStringList LoadingCache::imageFilePathsInCache() const
{
    d->cleanUpImageFilePathHash();
    return d->imageFilePathHash.uniqueKeys();
}

QStringList LoadingCache::thumbnailFilePathsInCache() const
{
    d->cleanUpThumbnailFilePathHash();
    return d->thumbnailFilePathHash.uniqueKeys();
}

void LoadingCache::notifyFileChanged(const QString& filePath)
{
    QList<QString> keys = d->imageFilePathHash.values(filePath);
    foreach(const QString& cacheKey, keys)
    {
        if (d->imageCache.remove(cacheKey))
        {
            emit fileChanged(filePath, cacheKey);
        }
    }

    keys = d->thumbnailFilePathHash.values(filePath);
    foreach(const QString& cacheKey, keys)
    {
        if (d->thumbnailImageCache.remove(cacheKey) ||
            d->thumbnailPixmapCache.remove(cacheKey))
        {
            emit fileChanged(filePath, cacheKey);
        }
    }

    emit fileChanged(filePath);
}

LoadingCacheFileWatch* LoadingCachePriv::fileWatch()
{
    // install default watch if no watch is set yet
    if (!watch)
    {
        q->setFileWatch(new ClassicLoadingCacheFileWatch);
    }

    return watch;
}

void LoadingCachePriv::mapImageFilePath(const QString& filePath, const QString& cacheKey)
{
    if (imageFilePathHash.size() > 5*imageCache.size())
    {
        cleanUpImageFilePathHash();
    }

    imageFilePathHash.insert(filePath, cacheKey);
}

void LoadingCachePriv::mapThumbnailFilePath(const QString& filePath, const QString& cacheKey)
{
    if (thumbnailFilePathHash.size() > 5*(thumbnailImageCache.size() + thumbnailPixmapCache.size()))
    {
        cleanUpThumbnailFilePathHash();
    }

    thumbnailFilePathHash.insert(filePath, cacheKey);
}

void LoadingCachePriv::cleanUpImageFilePathHash()
{
    // Remove all entries from hash whose value is no longer a key in the cache
    QSet<QString> keys = imageCache.keys().toSet();
    QMultiHash<QString, QString>::iterator it;

    for (it = imageFilePathHash.begin(); it != imageFilePathHash.end(); )
    {
        if (!keys.contains(it.value()))
        {
            it = imageFilePathHash.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void LoadingCachePriv::cleanUpThumbnailFilePathHash()
{
    QSet<QString> keys;
    keys += thumbnailImageCache.keys().toSet();
    keys += thumbnailPixmapCache.keys().toSet();
    QMultiHash<QString, QString>::iterator it;

    for (it = thumbnailFilePathHash.begin(); it != thumbnailFilePathHash.end(); )
    {
        if (!keys.contains(it.value()))
        {
            it = thumbnailFilePathHash.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void LoadingCache::iccSettingsChanged(const ICCSettingsContainer& current, const ICCSettingsContainer& previous)
{
    if (current.enableCM != previous.enableCM ||
        current.useManagedPreviews != previous.useManagedPreviews ||
        current.monitorProfile != previous.monitorProfile)
    {
        LoadingCache::CacheLock lock(this);
        removeImages();
        removeThumbnails();
    }
}

//---------------------------------------------------------------------------------------------------

LoadingCacheFileWatch::~LoadingCacheFileWatch()
{
    if (m_cache)
    {
        LoadingCache::CacheLock lock(m_cache);

        if (m_cache->d->watch == this)
        {
            m_cache->d->watch = 0;
        }
    }
}

void LoadingCacheFileWatch::notifyFileChanged(const QString& filePath)
{
    if (m_cache)
    {
        LoadingCache::CacheLock lock(m_cache);
        m_cache->notifyFileChanged(filePath);
    }
}

void LoadingCacheFileWatch::addedImage(const QString&)
{
    // default: do nothing
}

void LoadingCacheFileWatch::addedThumbnail(const QString&)
{
    // default: do nothing
}

//---------------------------------------------------------------------------------------------------

ClassicLoadingCacheFileWatch::ClassicLoadingCacheFileWatch()
{
    if (thread() != QCoreApplication::instance()->thread())
    {
        moveToThread(QCoreApplication::instance()->thread());
    }

    m_watch = new KDirWatch;

    connect(m_watch, SIGNAL(dirty(const QString&)),
            this, SLOT(slotFileDirty(const QString&)));

    // Make sure the signal gets here directly from the event loop.
    // If putImage is called from the main thread, with CacheLock,
    // a deadlock would result (mutex is not recursive)
    connect(this, SIGNAL(signalUpdateDirWatch()),
            this, SLOT(slotUpdateDirWatch()),
            Qt::QueuedConnection);

}

ClassicLoadingCacheFileWatch::~ClassicLoadingCacheFileWatch()
{
    delete m_watch;
}

void ClassicLoadingCacheFileWatch::addedImage(const QString& filePath)
{
    Q_UNUSED(filePath)
    // schedule update of file m_watch
    // KDirWatch can only be accessed from main thread!
    emit signalUpdateDirWatch();
}

void ClassicLoadingCacheFileWatch::addedThumbnail(const QString& filePath)
{
    Q_UNUSED(filePath);
    // ignore, we do not m_watch thumbnails
}

void ClassicLoadingCacheFileWatch::slotFileDirty(const QString& path)
{
    // Signal comes from main thread
    kDebug() << "LoadingCache slotFileDirty " << path;
    // This method acquires a lock itself
    notifyFileChanged(path);
    // No need for locking here, we are in main thread
    m_watch->removeFile(path);
    m_watchedFiles.removeAll(path);
}

void ClassicLoadingCacheFileWatch::slotUpdateDirWatch()
{
    // Event comes from main thread, we need to lock ourselves.
    LoadingCache::CacheLock lock(m_cache);

    // get a list of files in cache that need m_watch
    QStringList toBeAdded;
    QStringList toBeRemoved = m_watchedFiles;

    QList<QString> filePaths = m_cache->imageFilePathsInCache();
    foreach(const QString& m_watchPath, filePaths)
    {
        if (!m_watchPath.isEmpty())
        {
            if (!m_watchedFiles.contains(m_watchPath))
            {
                toBeAdded.append(m_watchPath);
            }

            toBeRemoved.removeAll(m_watchPath);
        }
    }

    for (QStringList::const_iterator it = toBeRemoved.constBegin(); it != toBeRemoved.constEnd(); ++it)
    {
        //kDebug() << "removing m_watch for " << *it;
        m_watch->removeFile(*it);
        m_watchedFiles.removeAll(*it);
    }

    for (QStringList::const_iterator it = toBeAdded.constBegin(); it != toBeAdded.constEnd(); ++it)
    {
        //kDebug() << "adding m_watch for " << *it;
        m_watch->addFile(*it);
        m_watchedFiles.append(*it);
    }
}

//---------------------------------------------------------------------------------------------------

LoadingCache::CacheLock::CacheLock(LoadingCache* cache)
    : m_cache(cache)
{
    m_cache->d->mutex.lock();
}

LoadingCache::CacheLock::~CacheLock()
{
    m_cache->d->mutex.unlock();
}

void LoadingCache::CacheLock::wakeAll()
{
    // obviously the mutex is locked when this function is called
    m_cache->d->condVar.wakeAll();
}

void LoadingCache::CacheLock::timedWait()
{
    // same as above, the mutex is certainly locked
    m_cache->d->condVar.wait(&m_cache->d->mutex, 1000);
}

}   // namespace Digikam
