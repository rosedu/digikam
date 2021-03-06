/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2005-02-17
 * Description : a plugin to transform image geometry.
 *
 * Copyright (C) 2005-2010 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#include "imageplugin_transform.moc"

// KDE includes

#include <kaction.h>
#include <kactioncollection.h>
#include <kapplication.h>
#include <kcursor.h>
#include <kgenericfactory.h>
#include <klibloader.h>
#include <klocale.h>
#include <kdebug.h>

// Local includes

#include "config-digikam.h"
#include "perspectivetool.h"
#include "freerotationtool.h"
#include "sheartool.h"
#include "resizetool.h"
#include "ratiocroptool.h"

#ifdef HAVE_GLIB2
#include "contentawareresizetool.h"
#endif /* HAVE_GLIB2 */

using namespace DigikamTransformImagePlugin;

K_PLUGIN_FACTORY( TransformFactory, registerPlugin<ImagePlugin_Transform>(); )
K_EXPORT_PLUGIN ( TransformFactory("digikamimageplugin_transform") )

class ImagePlugin_Transform::ImagePlugin_TransformPriv
{
public:

    ImagePlugin_TransformPriv() :
        aspectRatioCropAction(0),
        resizeAction(0),
        contentAwareResizingAction(0),
        sheartoolAction(0),
        freerotationAction(0),
        perspectiveAction(0)
    {
    }

    KAction* aspectRatioCropAction;
    KAction* resizeAction;
    KAction* contentAwareResizingAction;
    KAction* sheartoolAction;
    KAction* freerotationAction;
    KAction* perspectiveAction;
};

ImagePlugin_Transform::ImagePlugin_Transform(QObject* parent, const QVariantList&)
    : ImagePlugin(parent, "ImagePlugin_Transform"),
      d(new ImagePlugin_TransformPriv)
{
    d->perspectiveAction = new KAction(KIcon("perspective"), i18n("Perspective Adjustment..."), this);
    actionCollection()->addAction("imageplugin_perspective", d->perspectiveAction);
    connect(d->perspectiveAction, SIGNAL(triggered(bool)),
            this, SLOT(slotPerspective()));

    d->sheartoolAction = new KAction(KIcon("shear"), i18n("Shear..."), this);
    actionCollection()->addAction("imageplugin_sheartool", d->sheartoolAction);
    connect(d->sheartoolAction, SIGNAL(triggered(bool)),
            this, SLOT(slotShearTool()));

    d->resizeAction = new KAction(KIcon("transform-scale"), i18n("&Resize..."), this);
    actionCollection()->addAction("imageplugin_resize", d->resizeAction);
    connect(d->resizeAction, SIGNAL(triggered()),
            this, SLOT(slotResize()));

    d->aspectRatioCropAction = new KAction(KIcon("ratiocrop"), i18n("Aspect Ratio Crop..."), this);
    actionCollection()->addAction("imageplugin_ratiocrop", d->aspectRatioCropAction);
    connect(d->aspectRatioCropAction, SIGNAL(triggered(bool) ),
            this, SLOT(slotRatioCrop()));

#ifdef HAVE_GLIB2

    d->contentAwareResizingAction = new KAction(KIcon("transform-scale"), i18n("Liquid Rescale..."), this);
    // d->contentAwareResizingAction->setShortcut(KShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_C));
    actionCollection()->addAction("imageplugin_contentawareresizing", d->contentAwareResizingAction);
    connect(d->contentAwareResizingAction, SIGNAL(triggered(bool)),
            this, SLOT(slotContentAwareResizing()));

#endif /* HAVE_GLIB2 */

    //-----------------------------------------------------------------------------------

    QString pluginName(i18n("Free Rotation"));

    // we want to have an actionCategory for this plugin (if possible), set a name for it
    setActionCategory(pluginName);

    d->freerotationAction = new KAction(KIcon("freerotation"), QString("%1...").arg(pluginName), this);
    actionCollection()->addAction("imageplugin_freerotation", d->freerotationAction );
    connect(d->freerotationAction, SIGNAL(triggered(bool) ),
            this, SLOT(slotFreeRotation()));

    KAction* point1Action = new KAction(i18n("Set Point 1"), this);
    point1Action->setShortcut(KShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_1));
    actionCollection()->addAction("imageplugin_freerotation_point1", point1Action);
    connect(point1Action, SIGNAL(triggered(bool)),
            this, SIGNAL(signalPoint1Action()));

    KAction* point2Action = new KAction(i18n("Set Point 2"), this);
    point2Action->setShortcut(KShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_2));
    actionCollection()->addAction("imageplugin_freerotation_point2", point2Action);
    connect(point2Action, SIGNAL(triggered(bool)),
            this, SIGNAL(signalPoint2Action()));

    KAction* autoAdjustAction = new KAction(i18n("Auto Adjust"), this);
    autoAdjustAction->setShortcut(KShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_R));
    actionCollection()->addAction("imageplugin_freerotation_autoadjust", autoAdjustAction);
    connect(autoAdjustAction, SIGNAL(triggered(bool)),
            this, SIGNAL(signalAutoAdjustAction()));

    setXMLFile("digikamimageplugin_transform_ui.rc");

    kDebug() << "ImagePlugin_Transform plugin loaded";
}

ImagePlugin_Transform::~ImagePlugin_Transform()
{
    delete d;
}

void ImagePlugin_Transform::setEnabledActions(bool b)
{
    d->resizeAction->setEnabled(b);
    d->perspectiveAction->setEnabled(b);
    d->freerotationAction->setEnabled(b);
    d->sheartoolAction->setEnabled(b);
    d->aspectRatioCropAction->setEnabled(b);

#ifdef HAVE_GLIB2
    d->contentAwareResizingAction->setEnabled(b);
#endif /* HAVE_GLIB2 */
}

void ImagePlugin_Transform::slotPerspective()
{
    PerspectiveTool* tool = new PerspectiveTool(this);
    loadTool(tool);
}

void ImagePlugin_Transform::slotShearTool()
{
    ShearTool* tool = new ShearTool(this);
    loadTool(tool);
}

void ImagePlugin_Transform::slotResize()
{
    ResizeTool* tool = new ResizeTool(this);
    loadTool(tool);
}

void ImagePlugin_Transform::slotRatioCrop()
{
    RatioCropTool* tool = new RatioCropTool(this);
    loadTool(tool);
}

void ImagePlugin_Transform::slotContentAwareResizing()
{
#ifdef HAVE_GLIB2
    ContentAwareResizeTool* tool = new ContentAwareResizeTool(this);
    loadTool(tool);
#endif /* HAVE_GLIB2 */
}

void ImagePlugin_Transform::slotFreeRotation()
{
    FreeRotationTool* tool = new FreeRotationTool(this);

    connect(this, SIGNAL(signalPoint1Action()),
            tool, SLOT(slotAutoAdjustP1Clicked()));

    connect(this, SIGNAL(signalPoint2Action()),
            tool, SLOT(slotAutoAdjustP2Clicked()));

    connect(this, SIGNAL(signalAutoAdjustAction()),
            tool, SLOT(slotAutoAdjustClicked()));

    loadTool(tool);
}
