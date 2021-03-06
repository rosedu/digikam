/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2009-02-15
 * Description : contextmenu helper class
 *
 * Copyright (C) 2009-2010 by Andi Clemens <andi dot clemens at gmx dot net>
 * Copyright (C) 2010-2011 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#include "contextmenuhelper.moc"

// Qt includes

#include <QAction>
#include <QWidgetAction>
#include <QClipboard>
#include <QMap>
#include <QMenu>
#include <QMimeData>
#include <QPointer>
#include <QString>
#include <QTimer>

// KDE includes

#include <kaction.h>
#include <kactionmenu.h>
#include <kactioncollection.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kfileitem.h>
#include <klocale.h>
#include <kmenu.h>
#include <kmimetype.h>
#include <kmimetypetrader.h>
#include <kopenwithdialog.h>
#include <krun.h>
#include <kstandardaction.h>

#ifdef HAVE_KDEPIMLIBS
#include <kabc/stdaddressbook.h>
#endif // HAVE_KDEPIMLIBS

// LibKIPI includes

#include <libkipi/plugin.h>
#include <libkipi/pluginloader.h>

// Local includes

#include "album.h"
#include "albumdb.h"
#include "albummanager.h"
#include "config-digikam.h"
#include "databaseaccess.h"
#include "digikamapp.h"
#include "imageinfo.h"
#include "imagefiltermodel.h"
#include "lighttablewindow.h"
#include "queuemgrwindow.h"
#include "picklabelwidget.h"
#include "colorlabelwidget.h"
#include "ratingwidget.h"
#include "tagmodificationhelper.h"
#include "tagspopupmenu.h"

namespace Digikam
{

class ContextMenuHelper::ContextMenuHelperPriv
{
public:

    ContextMenuHelperPriv() :
        gotoAlbumAction(0),
        gotoDateAction(0),
        setThumbnailAction(0),
        imageFilterModel(0),
        parent(0),
        ABCmenu(0),
        stdActionCollection(0)
    {}

    QAction*                     gotoAlbumAction;
    QAction*                     gotoDateAction;
    QAction*                     setThumbnailAction;

    QList<qlonglong>             selectedIds;
    KUrl::List                   selectedItems;

    QMap<int, QAction*>          queueActions;
    QMap<QString, KService::Ptr> servicesMap;

    ImageFilterModel*            imageFilterModel;

    QMenu*                       parent;
    QMenu*                       ABCmenu;

    KActionCollection*           stdActionCollection;
};

ContextMenuHelper::ContextMenuHelper(QMenu* parent, KActionCollection* actionCollection)
    : QObject(parent), d(new ContextMenuHelperPriv)
{
    d->parent = parent;

    if (!actionCollection)
    {
        d->stdActionCollection = DigikamApp::instance()->actionCollection();
    }
    else
    {
        d->stdActionCollection = actionCollection;
    }
}

ContextMenuHelper::~ContextMenuHelper()
{
    delete d;
}

void ContextMenuHelper::addAction(const char* name, bool addDisabled)
{
    QAction* action = d->stdActionCollection->action(name);

    if (action)
    {
        addAction(action, addDisabled);
    }
}

void ContextMenuHelper::addAction(QAction* action, bool addDisabled)
{
    if (!action)
    {
        return;
    }

    if (action->isEnabled() || addDisabled)
    {
        d->parent->addAction(action);
    }
}

void ContextMenuHelper::addSubMenu(KMenu* subMenu)
{
    d->parent->addMenu(subMenu);
}

void ContextMenuHelper::addSeparator()
{
    d->parent->addSeparator();
}

void ContextMenuHelper::addAction(QAction* action, QObject* recv, const char* slot,
                                  bool addDisabled)
{
    if (!action)
    {
        return;
    }

    connect(action, SIGNAL(triggered()), recv, slot);
    addAction(action, addDisabled);
}

void ContextMenuHelper::addStandardActionLightTable()
{
    QAction* action = 0;
    QStringList ltActionNames;
    ltActionNames << QString("image_add_to_lighttable")
                  << QString("image_lighttable");

    if (LightTableWindow::lightTableWindowCreated() && !LightTableWindow::lightTableWindow()->isEmpty())
    {
        action = d->stdActionCollection->action(ltActionNames.at(0));
    }
    else
    {
        action = d->stdActionCollection->action(ltActionNames.at(1));
    }

    addAction(action);
}

void ContextMenuHelper::addStandardActionThumbnail(imageIds& ids, Album* album)
{
    if (d->setThumbnailAction)
    {
        return;
    }

    setSelectedIds(ids);

    if (album && ids.count() == 1)
    {
        if (album->type() == Album::PHYSICAL )
        {
            d->setThumbnailAction = new QAction(i18n("Set as Album Thumbnail"), this);
        }
        else if (album->type() == Album::TAG )
        {
            d->setThumbnailAction = new QAction(i18n("Set as Tag Thumbnail"), this);
        }

        addAction(d->setThumbnailAction);
        d->parent->addSeparator();
    }
}

void ContextMenuHelper::addServicesMenu(const KUrl::List& selectedItems)
{
    setSelectedItems(selectedItems);

    // This code is inspired by KonqMenuActions:
    // kdebase/apps/lib/konq/konq_menuactions.cpp

    QStringList    mimeTypes;
    KService::List offers;

    foreach(const KUrl& item, d->selectedItems)
    {
        const QString mimeType = KMimeType::findByUrl(item, 0, true, true)->name();

        if (!mimeTypes.contains(mimeType))
        {
            mimeTypes << mimeType;
        }
    }

    if (!mimeTypes.isEmpty())
    {
        // Query trader
        const QString firstMimeType = mimeTypes.takeFirst();
        const QString constraintTemplate = "'%1' in ServiceTypes";
        QStringList constraints;
        foreach(const QString& mimeType, mimeTypes)
        {
            constraints << constraintTemplate.arg(mimeType);
        }

        offers = KMimeTypeTrader::self()->query(firstMimeType, "Application", constraints.join( " and "));

        // remove duplicate service entries
        QSet<QString> seenApps;

        for (KService::List::iterator it = offers.begin(); it != offers.end();)
        {
            const QString appName((*it)->name());

            if (!seenApps.contains(appName))
            {
                seenApps.insert(appName);
                ++it;
            }
            else
            {
                it = offers.erase(it);
            }
        }
    }

    if (!offers.isEmpty())
    {
        KMenu* servicesMenu = new KMenu(d->parent);
        qDeleteAll(servicesMenu->actions());

        QAction* serviceAction = servicesMenu->menuAction();
        serviceAction->setText(i18n("Open With"));

        foreach (KService::Ptr service, offers)
        {
            QString name = service->name().replace( '&', "&&" );
            QAction* action = servicesMenu->addAction(name);
            action->setIcon(KIcon(service->icon()));
            action->setData(service->name());
            d->servicesMap[name] = service;
        }

        servicesMenu->addSeparator();
        servicesMenu->addAction(i18n("Other..."));

        addAction(serviceAction);

        connect(servicesMenu, SIGNAL(triggered(QAction*)),
                this, SLOT(slotOpenWith(QAction*)));
    }
    else
    {
        QAction* serviceAction = new QAction(i18n("Open With..."), this);
        addAction(serviceAction);

        connect(serviceAction, SIGNAL(triggered()),
                this, SLOT(slotOpenWith()));
    }
}

void ContextMenuHelper::slotOpenWith()
{
    // call the slot with an "empty" action
    slotOpenWith(0);
}

void ContextMenuHelper::slotOpenWith(QAction* action)
{
    KService::Ptr service;
    KUrl::List list = d->selectedItems;

    QString name = action ? action->data().toString() : QString();

    if (name.isEmpty())
    {
        QPointer<KOpenWithDialog> dlg = new KOpenWithDialog(list);

        if (!dlg->exec() == KOpenWithDialog::Accepted)
        {
            delete dlg;
            return;
        }

        service = dlg->service();

        if (!service)
        {
            // User entered a custom command
            if (!dlg->text().isEmpty())
            {
                KRun::run(dlg->text(), list, d->parent);
            }

            delete dlg;
            return;
        }

        delete dlg;
    }
    else
    {
        service = d->servicesMap[name];
    }

    KRun::run(*service, list, d->parent);
}

void ContextMenuHelper::addKipiActions(imageIds& ids)
{
    setSelectedIds(ids);

    if (imageIdsHaveSameCategory(ids, DatabaseItem::Image))
    {
        KAction* action = kipiRotateAction();

        if (action)
        {
            d->parent->addAction(action);
        }
    }
}

KAction* ContextMenuHelper::kipiRotateAction()
{
    KIPI::PluginLoader* kipiPluginLoader      = KIPI::PluginLoader::instance();
    KIPI::PluginLoader::PluginList pluginList = kipiPluginLoader->pluginList();

    foreach (KIPI::PluginLoader::Info* info, pluginList)
    {
        KIPI::Plugin* plugin = info->plugin();

        if (plugin && info->shouldLoad() && info->library() == "kipiplugin_jpeglossless")
        {
            QList<KAction*> actionList = plugin->actions();
            foreach (KAction* action, actionList)
            {
                if (action->objectName().toLatin1() == QString::fromLatin1("jpeglossless_rotate"))
                {
                    return(action);
                }
            }
        }
    }
    return 0;
}

bool ContextMenuHelper::imageIdsHaveSameCategory(const imageIds& ids, DatabaseItem::Category category)
{
    bool sameCategory = true;
    QVariantList varList;

    foreach (const qlonglong& id, ids)
    {
        varList = DatabaseAccess().db()->getImagesFields(id, DatabaseFields::Category);

        if ( varList.isEmpty() ||
             (DatabaseItem::Category)varList.first().toInt() != category )
        {
            sameCategory = false;
            break;
        }
    }
    return sameCategory;
}

void ContextMenuHelper::addActionNewTag(TagModificationHelper* helper)
{
    QAction* newTagAction = new QAction(SmallIcon("tag-new"), i18n("New Tag..."), this);
    addAction(newTagAction);
    connect(newTagAction, SIGNAL(triggered()),
            helper, SLOT(slotTagNew()));
}

void ContextMenuHelper::addActionDeleteTag(TagModificationHelper* helper)
{
    QAction* deleteTagAction = new QAction(SmallIcon("user-trash"), i18n("Delete Tag"), this);
    addAction(deleteTagAction);
    connect(deleteTagAction, SIGNAL(triggered()),
            helper, SLOT(slotTagDelete()));
}

void ContextMenuHelper::addActionEditTag(TagModificationHelper* helper)
{
    QAction* editTagAction = new QAction(SmallIcon("tag-properties"), i18nc("Edit Tag Properties", "Properties..."), this);
    addAction(editTagAction);
    connect(editTagAction, SIGNAL(triggered()),
            helper, SLOT(slotTagEdit()));
}

void ContextMenuHelper::addAssignTagsMenu(imageIds& ids)
{
    setSelectedIds(ids);

    KMenu* assignTagsPopup = new TagsPopupMenu(ids, TagsPopupMenu::RECENTLYASSIGNED, d->parent);
    assignTagsPopup->menuAction()->setText(i18n("Assign Tag"));
    assignTagsPopup->menuAction()->setIcon(SmallIcon("tag"));
    d->parent->addMenu(assignTagsPopup);

    connect(assignTagsPopup, SIGNAL(signalTagActivated(int)),
            this, SIGNAL(signalAssignTag(int)));

    connect(assignTagsPopup, SIGNAL(signalPopupTagsView()),
            this, SIGNAL(signalPopupTagsView()));
}

void ContextMenuHelper::addRemoveTagsMenu(imageIds& ids)
{
    setSelectedIds(ids);

    KMenu* removeTagsPopup = new TagsPopupMenu(ids, TagsPopupMenu::REMOVE, d->parent);
    removeTagsPopup->menuAction()->setText(i18n("Remove Tag"));
    removeTagsPopup->menuAction()->setIcon(SmallIcon("tag"));
    d->parent->addMenu(removeTagsPopup);

    // Performance: Only check for tags if there are <250 images selected
    // Also disable the remove Tag popup menu, if there are no tags at all.
    if (ids.count() > 250 || !DatabaseAccess().db()->hasTags(ids))
    {
        removeTagsPopup->menuAction()->setEnabled(false);
    }

    connect(removeTagsPopup, SIGNAL(signalTagActivated(int)),
            this, SIGNAL(signalRemoveTag(int)));
}

void ContextMenuHelper::addLabelsAction()
{
    KMenu* menuLabels           = new KMenu(i18n("Assign Labels"), d->parent);
    PickLabelMenuAction* pmenu  = new PickLabelMenuAction(d->parent);
    ColorLabelMenuAction* cmenu = new ColorLabelMenuAction(d->parent);
    RatingMenuAction* rmenu     = new RatingMenuAction(d->parent);
    menuLabels->addAction(pmenu);
    menuLabels->addAction(cmenu);
    menuLabels->addAction(rmenu);
    addSubMenu(menuLabels);

    connect(pmenu, SIGNAL(signalPickLabelChanged(int)),
            this, SIGNAL(signalAssignPickLabel(int)));

    connect(cmenu, SIGNAL(signalColorLabelChanged(int)),
            this, SIGNAL(signalAssignColorLabel(int)));

    connect(rmenu, SIGNAL(signalRatingChanged(int)),
            this, SIGNAL(signalAssignRating(int)));
}

void ContextMenuHelper::addCreateTagFromAddressbookMenu()
{
#ifdef HAVE_KDEPIMLIBS

    delete d->ABCmenu;

    d->ABCmenu = new QMenu(d->parent);

    connect(d->ABCmenu, SIGNAL(aboutToShow()),
            this, SLOT(slotABCContextMenu()));

    QAction* abcAction = d->ABCmenu->menuAction();
    abcAction->setIcon(SmallIcon("tag-addressbook"));
    abcAction->setText(i18n("Create Tag From Address Book"));
    d->parent->addMenu(d->ABCmenu);

    connect(d->ABCmenu, SIGNAL(triggered(QAction*)),
            this, SLOT(slotABCMenuTriggered(QAction*)));

#endif // HAVE_KDEPIMLIBS
}

//void ContextMenuHelper::slotABCContextMenu()
//{
//#ifdef HAVE_KDEPIMLIBS
//    d->ABCmenu->clear();
//
//    KABC::AddressBook* ab = KABC::StdAddressBook::self();
//    QStringList names;
//
//    for ( KABC::AddressBook::Iterator it = ab->begin(); it != ab->end(); ++it )
//    {
//        names.push_back(it->formattedName());
//    }
//
//    qSort(names);
//
//    for ( QStringList::ConstIterator it = names.constBegin(); it != names.constEnd(); ++it )
//    {
//        QString name = *it;
//
//        if (!name.isNull() )
//        {
//            d->ABCmenu->addAction(name);
//        }
//    }
//
//    if (d->ABCmenu->isEmpty())
//    {
//        QAction* nothingFound = d->ABCmenu->addAction(i18n("No address book entries found"));
//        nothingFound->setEnabled(false);
//    }
//
//#endif // HAVE_KDEPIMLIBS
//}

void ContextMenuHelper::slotABCMenuTriggered(QAction* action)
{
    QString name = action->iconText();
    emit signalAddNewTagFromABCMenu(name);
}

void ContextMenuHelper::slotDeselectAllAlbumItems()
{
    QAction* selectNoneAction = 0;
    selectNoneAction = d->stdActionCollection->action("selectNone");

    QTimer::singleShot(75, selectNoneAction, SIGNAL(triggered()));
}

void ContextMenuHelper::addImportMenu()
{
    KMenu* menuImport = new KMenu(i18n("Import"), d->parent);
    const QList<QAction*> importActions = DigikamApp::instance()->menuImportActions();

    if (!importActions.isEmpty())
    {
        menuImport->addActions(importActions);
    }

    d->parent->addMenu(menuImport);
}

void ContextMenuHelper::addExportMenu()
{
    KMenu* menuExport = new KMenu(i18n("Export"), d->parent);
    const QList<QAction*> exportActions = DigikamApp::instance()->menuExportActions();

    QAction* selectAllAction = 0;
    selectAllAction = d->stdActionCollection->action("selectAll");

    if (!exportActions.isEmpty())
    {
        menuExport->addActions(exportActions);
    }

    d->parent->addMenu(menuExport);
}

void ContextMenuHelper::addBatchMenu()
{
    KMenu* menuKIPIBatch = new KMenu(i18n("Batch Process"), d->parent);
    const QList<QAction*> batchActions = DigikamApp::instance()->menuBatchActions();

    QAction* selectAllAction = 0;
    selectAllAction = d->stdActionCollection->action("selectAll");

    if (!batchActions.isEmpty())
    {
        menuKIPIBatch->addActions(batchActions);
    }

    d->parent->addMenu(menuKIPIBatch);
}

void ContextMenuHelper::addAlbumActions()
{
    const QList<QAction*>& albumActions = DigikamApp::instance()->menuAlbumActions();

    if (!albumActions.isEmpty())
    {
        d->parent->addActions(albumActions);
    }
}

void ContextMenuHelper::addGotoMenu(imageIds& ids)
{
    if (d->gotoAlbumAction && d->gotoDateAction)
    {
        return;
    }

    setSelectedIds(ids);

    // the currently selected image is always the first item
    ImageInfo item;

    if (!d->selectedIds.isEmpty())
    {
        item = ImageInfo(d->selectedIds.first());
    }

    if (item.isNull())
    {
        return;
    }

    // when more then one item is selected, don't add the menu
    if (d->selectedIds.count() > 1)
    {
        return;
    }

    d->gotoAlbumAction = new QAction(SmallIcon("folder-image"),        i18n("Album"), this);
    d->gotoDateAction  = new QAction(SmallIcon("view-calendar-month"), i18n("Date"),  this);
    KMenu* gotoMenu    = new KMenu(d->parent);
    gotoMenu->addAction(d->gotoAlbumAction);
    gotoMenu->addAction(d->gotoDateAction);

    TagsPopupMenu* gotoTagsPopup = new TagsPopupMenu(d->selectedIds, TagsPopupMenu::DISPLAY, gotoMenu);
    QAction* gotoTag = gotoMenu->addMenu(gotoTagsPopup);
    gotoTag->setIcon(SmallIcon("tag"));
    gotoTag->setText(i18n("Tag"));

    // Disable the goto Tag popup menu, if there are no tags at all.
    if (!DatabaseAccess().db()->hasTags(d->selectedIds))
    {
        gotoTag->setEnabled(false);
    }

    Album* currentAlbum = AlbumManager::instance()->currentAlbum();

    if (currentAlbum->type() == Album::PHYSICAL)
    {
        // If the currently selected album is the same as album to
        // which the image belongs, then disable the "Go To" Album.
        // (Note that in recursive album view these can be different).
        if (item.albumId() == currentAlbum->id())
        {
            d->gotoAlbumAction->setEnabled(false);
        }
    }
    else if (currentAlbum->type() == Album::DATE)
    {
        d->gotoDateAction->setEnabled(false);
    }

    QAction* gotoMenuAction = gotoMenu->menuAction();
    gotoMenuAction->setIcon(SmallIcon("go-jump"));
    gotoMenuAction->setText(i18n("Go To"));

    connect(gotoTagsPopup, SIGNAL(signalTagActivated(int)),
            this, SIGNAL(signalGotoTag(int)));

    addAction(gotoMenuAction);
}

void ContextMenuHelper::addQueueManagerMenu()
{
    KMenu* bqmMenu = new KMenu(i18n("Batch Queue Manager"), d->parent);
    bqmMenu->menuAction()->setIcon(KIcon("bqm-diff"));
    bqmMenu->addAction(d->stdActionCollection->action("image_add_to_current_queue"));
    bqmMenu->addAction(d->stdActionCollection->action("image_add_to_new_queue"));

    // if queue list is empty, do not display the queue submenu
    if (QueueMgrWindow::queueManagerWindowCreated() &&
        !QueueMgrWindow::queueManagerWindow()->queuesMap().isEmpty())
    {
        QueueMgrWindow* qmw = QueueMgrWindow::queueManagerWindow();
        KMenu* queueMenu    = new KMenu(i18n("Add to Existing Queue"), bqmMenu);

        // queueActions is used by the exec() method to emit an appropriate signal.
        // Reset the map before filling in the actions.
        if (!d->queueActions.isEmpty())
        {
            d->queueActions.clear();
        }

        QList<QAction*> queueList;

        // get queue list from BQM window, do not access it directly, it might crash
        // when the list is changed
        QMap<int, QString> qmwMap = qmw->queuesMap();

        for (QMap<int, QString>::const_iterator it = qmwMap.constBegin(); it != qmwMap.constEnd(); ++it)
        {
            QAction* action = new QAction(it.value(), this);
            queueList << action;
            d->queueActions[it.key()] = action;
        }

        queueMenu->addActions(queueList);
        bqmMenu->addMenu(queueMenu);
    }

    d->parent->addMenu(bqmMenu);

    // NOTE: see B.K.O #252130 : we need to disable new items to add on BQM is this one is running.
    bqmMenu->setDisabled(QueueMgrWindow::queueManagerWindow()->isBusy());
}

void ContextMenuHelper::addGroupMenu(imageIds& ids)
{
    QList<QAction*> actions = groupMenuActions(ids);
    if (actions.isEmpty())
    {
        return;
    }
    KMenu* menu = new KMenu(i18n("Group"));
    foreach (QAction* action, actions)
    {
        menu->addAction(action);
    }
    d->parent->addMenu(menu);
}

void ContextMenuHelper::addGroupActions(imageIds& ids)
{
    foreach (QAction* action, groupMenuActions(ids))
    {
        d->parent->addAction(action);
    }
}

void ContextMenuHelper::setImageFilterModel(ImageFilterModel* model)
{
    d->imageFilterModel = model;
}

QList<QAction*> ContextMenuHelper::groupMenuActions(imageIds& ids)
{
    setSelectedIds(ids);

    QList<QAction*> actions;
    if (ids.isEmpty())
    {
        if (d->imageFilterModel)
        {
            if (!d->imageFilterModel->isAllGroupsOpen())
            {
                QAction* openAction = new QAction(i18nc("@action:inmenu", "Open All Groups"), this);
                connect(openAction, SIGNAL(triggered()), this, SLOT(slotOpenGroup()));
                actions << openAction;
            }
            else
            {
                QAction* closeAction = new QAction(i18nc("@action:inmenu", "Close All Groups"), this);
                connect(closeAction, SIGNAL(triggered()), this, SLOT(slotOpenGroup()));
                actions << closeAction;
            }
        }
        return actions;
    }

    ImageInfo info(ids.first());
    if (ids.size() == 1)
    {
        if (info.hasGroupedImages())
        {
            if (d->imageFilterModel)
            {
                if (!d->imageFilterModel->isGroupOpen(info.id()))
                {
                    QAction* action = new QAction(i18nc("@action:inmenu", "Show Grouped Images"), this);
                    connect(action, SIGNAL(triggered()), this, SLOT(slotOpenGroups()));
                    actions << action;
                }
                else
                {
                    QAction* action = new QAction(i18nc("@action:inmenu", "Hide Grouped Images"), this);
                    connect(action, SIGNAL(triggered()), this, SLOT(slotCloseGroups()));
                    actions << action;
                }
            }

            QAction* separator = new QAction(this);
            separator->setSeparator(true);
            actions << separator;

            QAction* clearAction = new QAction(i18nc("@action:inmenu", "Ungroup"), this);
            connect(clearAction, SIGNAL(triggered()), this, SIGNAL(signalUngroup()));
            actions << clearAction;
        }
        else if (info.isGrouped())
        {
            QAction* action = new QAction(i18nc("@action:inmenu", "Remove From Group"), this);
            connect(action, SIGNAL(triggered()), this, SIGNAL(signalRemoveFromGroup()));
            actions << action;

            // TODO: set as group leader / pick image
        }
    }
    else
    {
        QAction* closeAction = new QAction(i18nc("@action:inmenu", "Group Selected Here"), this);
        connect(closeAction, SIGNAL(triggered()), this, SIGNAL(signalCreateGroup()));
        actions << closeAction;

        QAction* separator = new QAction(this);
        separator->setSeparator(true);
        actions << separator;

        if (d->imageFilterModel)
        {
            QAction* openAction = new QAction(i18nc("@action:inmenu", "Show Grouped Images"), this);
            connect(openAction, SIGNAL(triggered()), this, SLOT(slotOpenGroups()));
            actions << openAction;

            QAction* closeAction = new QAction(i18nc("@action:inmenu", "Hide Grouped Images"), this);
            connect(closeAction, SIGNAL(triggered()), this, SLOT(slotCloseGroups()));
            actions << closeAction;

            QAction* separator2 = new QAction(this);
            separator2->setSeparator(true);
            actions << separator2;
        }

        QAction* removeAction = new QAction(i18nc("@action:inmenu", "Remove Selected From Groups"), this);
        connect(removeAction, SIGNAL(triggered()), this, SIGNAL(signalRemoveFromGroup()));
        actions << removeAction;

        QAction* clearAction = new QAction(i18nc("@action:inmenu", "Ungroup Selected"), this);
        connect(clearAction, SIGNAL(triggered()), this, SIGNAL(signalUngroup()));
        actions << clearAction;
    }
    return actions;
}

void ContextMenuHelper::setGroupsOpen(bool open)
{
    if (!d->imageFilterModel || d->selectedIds.isEmpty())
    {
        return;
    }
    GroupImageFilterSettings settings = d->imageFilterModel->groupImageFilterSettings();
    foreach (qlonglong id, d->selectedIds)
    {
        ImageInfo info(id);
        if (info.hasGroupedImages())
        {
            settings.setOpen(id, open);
        }
    }
    d->imageFilterModel->setGroupImageFilterSettings(settings);
}

void ContextMenuHelper::slotOpenGroups()
{
    setGroupsOpen(true);
}

void ContextMenuHelper::slotCloseGroups()
{
    setGroupsOpen(false);
}

void ContextMenuHelper::slotOpenAllGroups()
{
    if (!d->imageFilterModel)
    {
        return;
    }
    d->imageFilterModel->setAllGroupsOpen(true);
}

void ContextMenuHelper::slotCloseAllGroups()
{
    if (!d->imageFilterModel)
    {
        return;
    }
    d->imageFilterModel->setAllGroupsOpen(false);
}


void ContextMenuHelper::addStandardActionCut(QObject* recv, const char* slot)
{
    KAction* cut = KStandardAction::cut(recv, slot, d->parent);
    addAction(cut);
}

void ContextMenuHelper::addStandardActionCopy(QObject* recv, const char* slot)
{
    KAction* copy = KStandardAction::copy(recv, slot, d->parent);
    addAction(copy);
}

void ContextMenuHelper::addStandardActionPaste(QObject* recv, const char* slot)
{
    KAction* paste = KStandardAction::paste(recv, slot, d->parent);

    const QMimeData* data = kapp->clipboard()->mimeData(QClipboard::Clipboard);

    if (!data || !KUrl::List::canDecode(data))
    {
        paste->setEnabled(false);
    }

    addAction(paste);
}

void ContextMenuHelper::addStandardActionItemDelete(QObject* recv, const char* slot, int quantity)
{
    QAction* trashAction = new QAction(SmallIcon("user-trash"), i18ncp("@action:inmenu Pluralized",
                                       "Move to Trash", "Move %1 Files to Trash", quantity), d->parent);
    connect(trashAction, SIGNAL(triggered()),
            recv, slot);
    addAction(trashAction);
}

QAction* ContextMenuHelper::exec(const QPoint& pos, QAction* at)
{
    QAction* choice = d->parent->exec(pos, at);

    if (choice)
    {
        if (d->selectedIds.count() == 1)
        {
            ImageInfo selectedItem(d->selectedIds.first());

            if (choice == d->gotoAlbumAction)
            {
                emit signalGotoAlbum(selectedItem);
            }
            else if (choice == d->gotoDateAction)
            {
                emit signalGotoDate(selectedItem);
            }
            else if (choice == d->setThumbnailAction)
            {
                emit signalSetThumbnail(selectedItem);
            }
        }

        // check if a BQM action has been triggered
        for (QMap<int, QAction*>::iterator it = d->queueActions.begin();
             it != d->queueActions.end(); ++it)
        {
            if (choice == it.value())
            {
                emit signalAddToExistingQueue(it.key());
                return choice;
            }
        }
    }

    return choice;
}

void ContextMenuHelper::setSelectedIds(imageIds& ids)
{
    if (d->selectedIds.isEmpty())
    {
        d->selectedIds = ids;
    }
}

void ContextMenuHelper::setSelectedItems(const KUrl::List& urls)
{
    if (d->selectedItems.isEmpty())
    {
        d->selectedItems = urls;
    }
}

} // namespace Digikam
