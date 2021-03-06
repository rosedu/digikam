/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2010-04-30
 * Description : Graphics View items for DImg
 *
 * Copyright (C) 2010 by Marcel Wiesweg <marcel dot wiesweg at gmx dot de>
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

#ifndef DIMGPREVIEWITEM_H
#define DIMGPREVIEWITEM_H

// Qt includes

#include <QGraphicsItem>
#include <QObject>

// Local includes

#include "digikam_export.h"
#include "graphicsdimgitem.h"

namespace Digikam
{

class DImg;
class ImageInfo;
class LoadingDescription;

class DIGIKAM_EXPORT DImgPreviewItem : public GraphicsDImgItem
{
    Q_OBJECT

public:

    enum State
    {
        NoImage,
        Loading,
        ImageLoaded,
        ImageLoadingFailed
    };

public:

    DImgPreviewItem(QGraphicsItem* parent = 0);
    ~DImgPreviewItem();

    void setDisplayingWidget(QWidget* widget);
    void setLoadFullImageSize(bool b);
    void setExifRotate(bool rotate);

    QString path() const;
    void setPath(const QString& path);

    State state() const;
    bool isLoaded() const;
    void reload();

    void setPreloadPaths(const QStringList& pathsToPreload);

    QString userLoadingHint() const;

Q_SIGNALS:

    void stateChanged(int state);
    void loaded();
    void loadingFailed();

private Q_SLOTS:

    void slotGotImagePreview(const LoadingDescription& loadingDescription, const DImg& image);
    void preloadNext();
    void slotFileChanged(const QString& path);

private:

    class DImgPreviewItemPrivate;
    Q_DECLARE_PRIVATE(DImgPreviewItem)

protected:

    DImgPreviewItem(DImgPreviewItemPrivate& dd, QGraphicsItem* parent = 0);
};

} // namespace Digikam

#endif // DIMGPREVIEWITEM_H
