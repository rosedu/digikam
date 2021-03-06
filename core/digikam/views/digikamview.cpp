/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2002-16-10
 * Description : implementation of album view interface.
 *
 * Copyright (C) 2002-2005 by Renchi Raju <renchi@pooh.tam.uiuc.edu>
 * Copyright (C) 2002-2011 by Gilles Caulier <caulier dot gilles at gmail dot com>
 * Copyright (C) 2009-2011 by Johannes Wienke <languitar at semipol dot de>
 * Copyright (C) 2010-2011 by Andi Clemens <andi dot clemens at gmx dot net>
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

#include "digikamview.moc"

// KDE includes

#include <kapplication.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kmessagebox.h>
#include <ktoolinvocation.h>
#include <krun.h>

// Local includes

#include "albumhistory.h"
#include "albumsettings.h"
#include "stackedview.h"
#include "batchsyncmetadata.h"
#include "digikamapp.h"
#include "digikamimageview.h"
#include "dzoombar.h"
#include "imagealbummodel.h"
#include "imageinfoalbumsjob.h"
#include "imagepreviewview.h"
#include "imagepropertiessidebardb.h"
#include "imageviewutilities.h"
#include "filterstatusbar.h"
#include "leftsidebarwidgets.h"
#include "loadingcacheinterface.h"
#include "mapwidgetview.h"
#include "metadatasettings.h"
#include "globals.h"
#include "metadatahub.h"
#include "metadatamanager.h"
#include "queuemgrwindow.h"
#include "scancontroller.h"
#include "sidebar.h"
#include "slideshow.h"
#include "statusprogressbar.h"
#include "filtersidebarwidget.h"
#include "tagmodificationhelper.h"
#include "imagepropertiesversionstab.h"
#include "tagscache.h"
#include "searchxml.h"
#include "faceiface.h"
#include "versionmanagersettings.h"

namespace Digikam
{

class DigikamView::DigikamViewPriv
{
public:

    DigikamViewPriv() :
        needDispatchSelection(false),
        cancelSlideShow(false),
        useAlbumHistory(false),
        initialAlbumID(0),
        thumbSize(ThumbnailSize::Medium),
        dockArea(0),
        splitter(0),
        selectionTimer(0),
        thumbSizeTimer(0),
        albumFolderSideBar(0),
        tagViewSideBar(0),
        dateViewSideBar(0),
        timelineSideBar(0),
        searchSideBar(0),
        fuzzySearchSideBar(0),
        gpsSearchSideBar(0),
        parent(0),
        iconView(0),
        albumManager(0),
        albumHistory(0),
        stackedview(0),
        lastPreviewMode(StackedView::PreviewAlbumMode),
        albumModificationHelper(0),
        tagModificationHelper(0),
        searchModificationHelper(0),
        leftSideBar(0),
        rightSideBar(0),
        filterWidget(0),
        optionAlbumViewPrefix("AlbumView"),
        modelCollection(0)
    {
    }

    QString                       userPresentableAlbumTitle(const QString& album);

    bool                          needDispatchSelection;
    bool                          cancelSlideShow;
    bool                          useAlbumHistory;

    int                           initialAlbumID;
    int                           thumbSize;

    QMainWindow*                  dockArea;

    SidebarSplitter*              splitter;

    QTimer*                       selectionTimer;
    QTimer*                       thumbSizeTimer;

    // left side bar
    AlbumFolderViewSideBarWidget* albumFolderSideBar;
    TagViewSideBarWidget*         tagViewSideBar;
    DateFolderViewSideBarWidget*  dateViewSideBar;
    TimelineSideBarWidget*        timelineSideBar;
    SearchSideBarWidget*          searchSideBar;
    FuzzySearchSideBarWidget*     fuzzySearchSideBar;

    GPSSearchSideBarWidget*       gpsSearchSideBar;

    PeopleSideBarWidget*          peopleSideBar;

    DigikamApp*                   parent;

    DigikamImageView*             iconView;
    MapWidgetView*                mapView;
    AlbumManager*                 albumManager;
    AlbumHistory*                 albumHistory;
    StackedView*                  stackedview;
    int                           lastPreviewMode;

    AlbumModificationHelper*      albumModificationHelper;
    TagModificationHelper*        tagModificationHelper;
    SearchModificationHelper*     searchModificationHelper;

    Sidebar*                      leftSideBar;
    ImagePropertiesSideBarDB*     rightSideBar;

    FilterSideBarWidget*          filterWidget;

    QString                       optionAlbumViewPrefix;

    QList<SidebarWidget*>         leftSideBarWidgets;

    DigikamModelCollection*       modelCollection;
};

DigikamView::DigikamView(QWidget* parent, DigikamModelCollection* modelCollection)
    : KHBox(parent), d(new DigikamViewPriv)
{
    d->parent          = static_cast<DigikamApp*>(parent);
    d->modelCollection = modelCollection;
    d->albumManager    = AlbumManager::instance();

    d->albumModificationHelper  = new AlbumModificationHelper(this, this);
    d->tagModificationHelper    = new TagModificationHelper(this, this);
    d->searchModificationHelper = new SearchModificationHelper(this, this);

    d->splitter = new SidebarSplitter;
    d->splitter->setFrameStyle( QFrame::NoFrame );
    d->splitter->setFrameShadow( QFrame::Plain );
    d->splitter->setFrameShape( QFrame::NoFrame );
    d->splitter->setOpaqueResize(false);

    d->leftSideBar = new Sidebar(this, d->splitter, KMultiTabBar::Left);
    d->leftSideBar->setObjectName("Digikam Left Sidebar");
    d->splitter->setParent(this);

    // The dock area where the thumbnail bar is allowed to go.
    d->dockArea    = new QMainWindow(this, Qt::Widget);
    d->splitter->addWidget(d->dockArea);
    d->stackedview = new StackedView(d->dockArea);
    d->dockArea->setCentralWidget(d->stackedview);
    d->stackedview->setDockArea(d->dockArea);

    d->iconView = d->stackedview->imageIconView();
    d->mapView = d->stackedview->mapWidgetView();

    d->rightSideBar = new ImagePropertiesSideBarDB(this, d->splitter, KMultiTabBar::Right, true);
    d->rightSideBar->setObjectName("Digikam Right Sidebar");

    // album folder view
    d->albumFolderSideBar = new AlbumFolderViewSideBarWidget(d->leftSideBar,
            d->modelCollection->getAlbumModel(),
            d->albumModificationHelper);
    d->leftSideBarWidgets << d->albumFolderSideBar;
    connect(d->albumFolderSideBar, SIGNAL(signalFindDuplicatesInAlbum(Album*)),
            this, SLOT(slotNewDuplicatesSearch(Album*)));

    // Tags sidebar tab contents.
    d->tagViewSideBar = new TagViewSideBarWidget(d->leftSideBar,
            d->modelCollection->getTagModel());
    d->leftSideBarWidgets << d->tagViewSideBar;
    connect(d->tagViewSideBar, SIGNAL(signalFindDuplicatesInAlbum(Album*)),
            this, SLOT(slotNewDuplicatesSearch(Album*)));

    // date view
    d->dateViewSideBar = new DateFolderViewSideBarWidget(d->leftSideBar,
            d->modelCollection->getDateAlbumModel(),
            d->iconView->imageAlbumFilterModel());
    d->leftSideBarWidgets << d->dateViewSideBar;

    // timeline side bar
    d->timelineSideBar = new TimelineSideBarWidget(d->leftSideBar,
            d->modelCollection->getSearchModel(),
            d->searchModificationHelper);
    d->leftSideBarWidgets << d->timelineSideBar;

    // Search sidebar tab contents.
    d->searchSideBar = new SearchSideBarWidget(d->leftSideBar,
            d->modelCollection->getSearchModel(),
            d->searchModificationHelper);
    d->leftSideBarWidgets << d->searchSideBar;

    // Fuzzy search
    d->fuzzySearchSideBar = new FuzzySearchSideBarWidget(d->leftSideBar,
            d->modelCollection->getSearchModel(),
            d->searchModificationHelper);
    d->leftSideBarWidgets << d->fuzzySearchSideBar;

    d->gpsSearchSideBar = new GPSSearchSideBarWidget(d->leftSideBar,
            d->modelCollection->getSearchModel(),
            d->searchModificationHelper,
            d->iconView->imageFilterModel(),d->iconView->getSelectionModel());

    d->leftSideBarWidgets << d->gpsSearchSideBar;

    // People Sidebar
    d->peopleSideBar = new PeopleSideBarWidget(d->leftSideBar,
            d->modelCollection->getTagModel(),
            d->searchModificationHelper);
    connect(d->peopleSideBar, SIGNAL(requestFaceMode(bool)),
            d->iconView, SLOT(setFaceMode(bool)));

    d->leftSideBarWidgets << d->peopleSideBar;

    foreach (SidebarWidget* leftWidget, d->leftSideBarWidgets)
    {
        d->leftSideBar->appendTab(leftWidget, leftWidget->getIcon(),
                                  leftWidget->getCaption());
        connect(leftWidget, SIGNAL(requestActiveTab(SidebarWidget*)),
                this, SLOT(slotLeftSideBarActivate(SidebarWidget*)));
    }

    // To the right.

    // Tags Filter sidebar tab contents.
    d->filterWidget = new FilterSideBarWidget(d->rightSideBar, d->modelCollection->getTagFilterModel());
    d->rightSideBar->appendTab(d->filterWidget, SmallIcon("view-filter"), i18n("Filters"));

    // Versions sidebar overlays
    d->rightSideBar->getFiltersHistoryTab()->addOpenAlbumAction(d->iconView->imageModel());
    d->rightSideBar->getFiltersHistoryTab()->addShowHideOverlay();

    d->selectionTimer = new QTimer(this);
    d->selectionTimer->setSingleShot(true);
    d->selectionTimer->setInterval(75);
    d->thumbSizeTimer = new QTimer(this);
    d->thumbSizeTimer->setSingleShot(true);
    d->thumbSizeTimer->setInterval(300);

    d->albumHistory = new AlbumHistory();

    slotSidebarTabTitleStyleChanged();
    setupConnections();
}

DigikamView::~DigikamView()
{
    saveViewState();

    delete d->albumHistory;
    delete d;
}

void DigikamView::applySettings()
{
    foreach (SidebarWidget* sidebarWidget, d->leftSideBarWidgets)
    {
        sidebarWidget->applySettings();
    }

    d->iconView->imageFilterModel()->setVersionImageFilterSettings(AlbumSettings::instance()->getVersionManagerSettings());

    refreshView();
}

void DigikamView::refreshView()
{
    d->rightSideBar->refreshTagsView();
}

void DigikamView::setupConnections()
{
    // -- DigikamApp connections ----------------------------------

    connect(d->parent, SIGNAL(signalEscapePressed()),
            this, SLOT(slotEscapePreview()));

    connect(d->parent, SIGNAL(signalEscapePressed()),
            d->stackedview, SLOT(slotEscapePreview()));

    connect(d->parent, SIGNAL(signalNextItem()),
            this, SLOT(slotNextItem()));

    connect(d->parent, SIGNAL(signalPrevItem()),
            this, SLOT(slotPrevItem()));

    connect(d->parent, SIGNAL(signalFirstItem()),
            this, SLOT(slotFirstItem()));

    connect(d->parent, SIGNAL(signalLastItem()),
            this, SLOT(slotLastItem()));

    connect(d->parent, SIGNAL(signalCutAlbumItemsSelection()),
            d->iconView, SLOT(cut()));

    connect(d->parent, SIGNAL(signalCopyAlbumItemsSelection()),
            d->iconView, SLOT(copy()));

    connect(d->parent, SIGNAL(signalPasteAlbumItemsSelection()),
            d->iconView, SLOT(paste()));

    connect(this, SIGNAL(signalProgressBarMode(int, const QString&)),
            d->parent, SLOT(slotProgressBarMode(int, const QString&)));

    connect(this, SIGNAL(signalProgressValue(int)),
            d->parent, SLOT(slotProgressValue(int)));

    connect(d->parent, SIGNAL(signalCancelButtonPressed()),
            this, SLOT(slotCancelSlideShow()));

    // -- AlbumManager connections --------------------------------

    connect(d->albumManager, SIGNAL(signalAlbumCurrentChanged(Album*)),
            this, SLOT(slotAlbumSelected(Album*)));

    connect(d->albumManager, SIGNAL(signalAllAlbumsLoaded()),
            this, SLOT(slotAllAlbumsLoaded()));

    connect(d->albumManager, SIGNAL(signalAlbumAdded(Album*)),
            this, SLOT(slotAlbumAdded(Album*)));

    connect(d->albumManager, SIGNAL(signalAlbumDeleted(Album*)),
            this, SLOT(slotAlbumDeleted(Album*)));

    connect(d->albumManager, SIGNAL(signalAlbumRenamed(Album*)),
            this, SLOT(slotAlbumRenamed(Album*)));

    connect(d->albumManager, SIGNAL(signalAlbumsCleared()),
            this, SLOT(slotAlbumsCleared()));

    // -- IconView Connections -------------------------------------

    connect(d->iconView->model(), SIGNAL(rowsInserted(const QModelIndex&, int, int)),
            this, SLOT(slotImageSelected()));

    connect(d->iconView->model(), SIGNAL(rowsRemoved(const QModelIndex&, int, int)),
            this, SLOT(slotImageSelected()));

    connect(d->iconView->model(), SIGNAL(layoutChanged()),
            this, SLOT(slotImageSelected()));

    connect(d->iconView, SIGNAL(selectionChanged()),
            this, SLOT(slotImageSelected()));

    connect(d->iconView, SIGNAL(previewRequested(const ImageInfo&)),
            this, SLOT(slotTogglePreviewMode(const ImageInfo&)));

    connect(d->iconView, SIGNAL(gotoAlbumAndImageRequested(const ImageInfo&)),
            this, SLOT(slotGotoAlbumAndItem(const ImageInfo&)));

    connect(d->iconView, SIGNAL(gotoDateAndImageRequested(const ImageInfo&)),
            this, SLOT(slotGotoDateAndItem(const ImageInfo&)));

    connect(d->iconView, SIGNAL(gotoTagAndImageRequested(int)),
            this, SLOT(slotGotoTagAndItem(int)));

    connect(d->iconView, SIGNAL(zoomOutStep()),
            this, SLOT(slotZoomOut()));

    connect(d->iconView, SIGNAL(zoomInStep()),
            this, SLOT(slotZoomIn()));

    connect(d->iconView, SIGNAL(signalPopupTagsView()),
            d->rightSideBar, SLOT(slotPopupTagsView()));

    // -- Sidebar Connections -------------------------------------

    connect(d->leftSideBar, SIGNAL(signalChangedTab(QWidget*)),
            this, SLOT(slotLeftSidebarChangedTab(QWidget*)));

    connect(d->rightSideBar, SIGNAL(signalFirstItem()),
            this, SLOT(slotFirstItem()));

    connect(d->rightSideBar, SIGNAL(signalNextItem()),
            this, SLOT(slotNextItem()));

    connect(d->rightSideBar, SIGNAL(signalPrevItem()),
            this, SLOT(slotPrevItem()));

    connect(d->rightSideBar, SIGNAL(signalLastItem()),
            this, SLOT(slotLastItem()));

    connect(this, SIGNAL(signalNoCurrentItem()),
            d->rightSideBar, SLOT(slotNoCurrentItem()));

    connect(d->rightSideBar, SIGNAL(signalProgressBarMode(int, const QString&)),
            d->parent, SLOT(slotProgressBarMode(int, const QString&)));

    connect(d->rightSideBar, SIGNAL(signalProgressValue(int)),
            d->parent, SLOT(slotProgressValue(int)));

    connect(d->fuzzySearchSideBar, SIGNAL(signalUpdateFingerPrints()),
            d->parent, SLOT(slotRebuildFingerPrints()));

    connect(d->fuzzySearchSideBar, SIGNAL(signalGenerateFingerPrintsFirstTime()),
            d->parent, SLOT(slotGenerateFingerPrintsFirstTime()));

    connect(d->peopleSideBar, SIGNAL(signalDetectFaces()),
            d->parent, SLOT(slotScanForFaces()));
    /*
        connect(d->fuzzySearchSideBar, SIGNAL(signalGenerateFingerPrintsFirstTime()),
                d->parent, SLOT(slotGenerateFingerPrintsFirstTime()));
    */

    connect(d->gpsSearchSideBar, SIGNAL(signalMapSoloItems(const QList<qlonglong>&, const QString&)),
            d->iconView->imageFilterModel(), SLOT(setIdWhitelist(const QList<qlonglong>&, const QString&)));

    // -- Filter Bars Connections ---------------------------------

    ImageAlbumFilterModel* model = d->iconView->imageAlbumFilterModel();

    connect(d->filterWidget,
            SIGNAL(signalTagFilterChanged(const QList<int>&, const QList<int>&,
                                          ImageFilterSettings::MatchingCondition, bool, const QList<int>&, const QList<int>&)),
            d->iconView->imageFilterModel(),
            SLOT(setTagFilter(const QList<int>&, const QList<int>&,
                              ImageFilterSettings::MatchingCondition, bool, const QList<int>&, const QList<int>&)));

    connect(d->filterWidget, SIGNAL(signalRatingFilterChanged(int, ImageFilterSettings::RatingCondition)),
            model, SLOT(setRatingFilter(int, ImageFilterSettings::RatingCondition)));

    connect(d->filterWidget, SIGNAL(signalSearchTextFilterChanged(const SearchTextFilterSettings&)),
            model, SLOT(setTextFilter(const SearchTextFilterSettings&)));

    connect(model, SIGNAL(filterMatchesForText(bool)),
            d->filterWidget, SLOT(slotFilterMatchesForText(bool)));

    connect(d->filterWidget, SIGNAL(signalMimeTypeFilterChanged(int)),
            model, SLOT(setMimeTypeFilter(int)));

    // -- Preview image widget Connections ------------------------

    connect(d->stackedview, SIGNAL(signalNextItem()),
            this, SLOT(slotNextItem()));

    connect(d->stackedview, SIGNAL(signalPrevItem()),
            this, SLOT(slotPrevItem()));

    connect(d->stackedview, SIGNAL(signalEditItem()),
            this, SLOT(slotImageEdit()));

    connect(d->stackedview, SIGNAL(signalDeleteItem()),
            this, SLOT(slotImageDelete()));

    connect(d->stackedview, SIGNAL(signalViewModeChanged()),
            this, SLOT(slotViewModeChanged()));

    connect(d->stackedview, SIGNAL(signalBack2Album()),
            this, SLOT(slotEscapePreview()));

    connect(d->stackedview, SIGNAL(signalSlideShow()),
            this, SLOT(slotSlideShowAll()));

    connect(d->stackedview, SIGNAL(signalZoomFactorChanged(double)),
            this, SLOT(slotZoomFactorChanged(double)));

    connect(d->stackedview, SIGNAL(signalInsert2LightTable()),
            this, SLOT(slotImageAddToLightTable()));

    connect(d->stackedview, SIGNAL(signalInsert2QueueMgr()),
            this, SLOT(slotImageAddToCurrentQueue()));

    connect(d->stackedview, SIGNAL(signalFindSimilar()),
            this, SLOT(slotImageFindSimilar()));

    connect(d->stackedview, SIGNAL(signalAddToExistingQueue(int)),
            this, SLOT(slotImageAddToExistingQueue(int)));

    connect(d->stackedview, SIGNAL(signalGotoAlbumAndItem(const ImageInfo&)),
            this, SLOT(slotGotoAlbumAndItem(const ImageInfo&)));

    connect(d->stackedview, SIGNAL(signalGotoDateAndItem(const ImageInfo&)),
            this, SLOT(slotGotoDateAndItem(const ImageInfo&)));

    connect(d->stackedview, SIGNAL(signalGotoTagAndItem(int)),
            this, SLOT(slotGotoTagAndItem(int)));

    // -- MetadataManager progress ---------------

    connect(MetadataManager::instance(), SIGNAL(progressMessageChanged(const QString&)),
            this, SLOT(slotProgressMessageChanged(const QString&)));

    connect(MetadataManager::instance(), SIGNAL(progressValueChanged(float)),
            this, SLOT(slotProgressValueChanged(float)));

    connect(MetadataManager::instance(), SIGNAL(progressFinished()),
            this, SLOT(slotProgressFinished()));

    connect(MetadataManager::instance(), SIGNAL(orientationChangeFailed(const QStringList&)),
            this, SLOT(slotOrientationChangeFailed(const QStringList&)));

    // -- timers ---------------

    connect(d->selectionTimer, SIGNAL(timeout()),
            this, SLOT(slotDispatchImageSelected()));

    connect(d->thumbSizeTimer, SIGNAL(timeout()),
            this, SLOT(slotThumbSizeEffect()) );

    // -- Album Settings ----------------

    connect(AlbumSettings::instance(), SIGNAL(setupChanged()),
            this, SLOT(slotSidebarTabTitleStyleChanged()));

    // -- Album History -----------------
    connect(this, SIGNAL(signalAlbumSelected(bool)),
            d->albumHistory, SLOT(slotAlbumSelected()));

    connect(this, SIGNAL(signalImageSelected(const ImageInfoList&, bool, bool, const ImageInfoList&)),
            d->albumHistory, SLOT(slotImageSelected(const ImageInfoList&)));

    connect(d->iconView, SIGNAL(currentChanged(const ImageInfo&)),
            d->albumHistory, SLOT(slotCurrentChange(const ImageInfo&)));

    connect(d->iconView, SIGNAL(gotoAlbumAndImageRequested(const ImageInfo&)),
            d->albumHistory, SLOT(slotClearSelectPAlbum(const ImageInfo&)));

    connect(d->iconView, SIGNAL(gotoTagAndImageRequested(int)),
            d->albumHistory, SLOT(slotClearSelectTAlbum(int)));

    connect(d->iconView->imageModel(), SIGNAL(imageInfosAdded(QList<ImageInfo>)),
            d->albumHistory, SLOT(slotAlbumCurrentChanged()));

    connect(d->albumHistory, SIGNAL(signalSetCurrent(qlonglong)),
            d->iconView, SLOT(setCurrentWhenAvailable(qlonglong)));

    connect(d->albumHistory, SIGNAL(signalSetSelectedInfos(const QList<ImageInfo>&)),
            d->iconView, SLOT(setSelectedImageInfos(const QList<ImageInfo>&)));

    connect(d->albumManager, SIGNAL(signalAlbumDeleted(Album*)),
            d->albumHistory, SLOT(slotAlbumDeleted(Album*)));

    // -- Image versions ----------------

    connect(d->rightSideBar->getFiltersHistoryTab(), SIGNAL(imageSelected(const ImageInfo&)),
            d->iconView, SLOT(hintAt(const ImageInfo&)));

    connect(d->rightSideBar->getFiltersHistoryTab(), SIGNAL(actionTriggered(const ImageInfo&)),
            this, SLOT(slotGotoAlbumAndItem(const ImageInfo&)));
}

void DigikamView::connectIconViewFilter(FilterStatusBar* filterbar)
{
    ImageAlbumFilterModel* model = d->iconView->imageAlbumFilterModel();

    connect(model, SIGNAL(filterMatches(bool)),
            filterbar, SLOT(slotFilterMatches(bool)));

    connect(model, SIGNAL(filterSettingsChanged(const ImageFilterSettings&)),
            filterbar, SLOT(slotFilterSettingsChanged(const ImageFilterSettings&)));

    connect(filterbar, SIGNAL(signalResetFilters()),
            d->filterWidget, SLOT(slotResetFilters()));

    connect(filterbar, SIGNAL(signalPopupFiltersView()),
            this, SLOT(slotPopupFiltersView()));

}

void DigikamView::slotPopupFiltersView()
{
    d->rightSideBar->setActiveTab(d->filterWidget);
    d->filterWidget->setFocusToTextFilter();
}

void DigikamView::loadViewState()
{
    foreach (SidebarWidget* widget, d->leftSideBarWidgets)
    {
        widget->loadState();
    }

    d->filterWidget->loadState();

    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup group        = config->group("MainWindow");

    // Restore the splitter
    d->splitter->restoreState(group);

    // Restore the thumbnail bar dock.
    QByteArray thumbbarState;
    thumbbarState = group.readEntry("ThumbbarState", thumbbarState);
    d->dockArea->restoreState(QByteArray::fromBase64(thumbbarState));

    d->initialAlbumID = group.readEntry("InitialAlbumID", 0);

    d->mapView->loadState();
    d->rightSideBar->loadState();
}

void DigikamView::saveViewState()
{
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup group        = config->group("MainWindow");

    foreach (SidebarWidget* widget, d->leftSideBarWidgets)
    {
        widget->saveState();
    }

    d->filterWidget->saveState();

    // Save the splitter states.
    d->splitter->saveState(group);

    // Save the position and size of the thumbnail bar. The thumbnail bar dock
    // needs to be closed explicitly, because when it is floating and visible
    // (when the user is in image preview mode) when the layout is saved, it
    // also reappears when restoring the view, while it should always be hidden.
    d->stackedview->thumbBarDock()->close();
    group.writeEntry("ThumbbarState", d->dockArea->saveState().toBase64());

    Album* album = AlbumManager::instance()->currentAlbum();

    if (album)
    {
        group.writeEntry("InitialAlbumID", album->globalID());
    }
    else
    {
        group.writeEntry("InitialAlbumID", 0);
    }

    d->mapView->saveState();

    d->rightSideBar->saveState();
}

QList<SidebarWidget*> DigikamView::leftSidebarWidgets()
{
    return d->leftSideBarWidgets;
}

KUrl::List DigikamView::allUrls() const
{
    return d->iconView->urls();
}

KUrl::List DigikamView::selectedUrls() const
{
    return d->iconView->selectedUrls();
}

void DigikamView::showSideBars()
{
    d->leftSideBar->restore();
    d->rightSideBar->restore();
}

void DigikamView::hideSideBars()
{
    d->leftSideBar->backup();
    d->rightSideBar->backup();
}

void DigikamView::slotFirstItem()
{
    d->iconView->toFirstIndex();
}

void DigikamView::slotPrevItem()
{
    d->iconView->toPreviousIndex();
}

void DigikamView::slotNextItem()
{
    d->iconView->toNextIndex();
}

void DigikamView::slotLastItem()
{
    d->iconView->toLastIndex();
}

void DigikamView::slotSelectItemByUrl(const KUrl& url)
{
    d->iconView->toIndex(url);
}

void DigikamView::slotAllAlbumsLoaded()
{
    disconnect(d->albumManager, SIGNAL(signalAllAlbumsLoaded()),
               this, SLOT(slotAllAlbumsLoaded()));

    loadViewState();
    d->leftSideBar->loadState();
    d->rightSideBar->loadState();
    d->rightSideBar->populateTags();

    // now that all albums have been loaded, activate the albumHistory
    d->useAlbumHistory = true;
    Album* album = d->albumManager->findAlbum(d->initialAlbumID);
    d->albumManager->setCurrentAlbum(album);
}

void DigikamView::slotSortAlbums(int order)
{
    AlbumSettings* settings = AlbumSettings::instance();

    if (!settings)
    {
        return;
    }

    settings->setAlbumSortOrder((AlbumSettings::AlbumSortOrder) order);
    // TODO sorting by anything else then the name is currently not supported by the model
    //d->folderView->resort();
}

void DigikamView::slotNewAlbum()
{
    // TODO use the selection model of the view instead
    d->albumModificationHelper->slotAlbumNew(d->albumFolderSideBar->currentAlbum());
}

void DigikamView::slotDeleteAlbum()
{
    d->albumModificationHelper->slotAlbumDelete(d->albumFolderSideBar->currentAlbum());
}

void DigikamView::slotNewTag()
{
    d->tagModificationHelper->slotTagNew(d->tagViewSideBar->currentAlbum());
}

void DigikamView::slotDeleteTag()
{
    d->tagModificationHelper->slotTagDelete(d->tagViewSideBar->currentAlbum());
}

void DigikamView::slotEditTag()
{
    d->tagModificationHelper->slotTagEdit(d->tagViewSideBar->currentAlbum());
}

void DigikamView::slotNewKeywordSearch()
{
    slotLeftSideBarActivate(d->searchSideBar);
    d->searchSideBar->newKeywordSearch();
}

void DigikamView::slotNewAdvancedSearch()
{
    slotLeftSideBarActivate(d->searchSideBar);
    d->searchSideBar->newAdvancedSearch();
}

void DigikamView::slotNewDuplicatesSearch(Album* album)
{
    slotLeftSideBarActivate(d->fuzzySearchSideBar);
    d->fuzzySearchSideBar->newDuplicatesSearch(album);
}

void DigikamView::slotAlbumAdded(Album* album)
{
    Q_UNUSED(album);
    // right now nothing has to be done here anymore
}

void DigikamView::slotAlbumDeleted(Album* album)
{
    d->albumHistory->deleteAlbum(album);
}

void DigikamView::slotAlbumRenamed(Album* album)
{
    Q_UNUSED(album);
}

void DigikamView::slotAlbumsCleared()
{
    emit signalAlbumSelected(false);
}

void DigikamView::slotAlbumHistoryBack(int steps)
{
    Album* album    = 0;
    QWidget* widget = 0;

    d->albumHistory->back(&album, &widget, steps);

    changeAlbumFromHistory(album, widget);
}

void DigikamView::slotAlbumHistoryForward(int steps)
{
    Album* album    = 0;
    QWidget* widget = 0;

    d->albumHistory->forward(&album, &widget, steps);

    changeAlbumFromHistory(album, widget);
}

// TODO update, use SideBarWidget instead of QWidget
void DigikamView::changeAlbumFromHistory(Album* album, QWidget* widget)
{
    if (album && widget)
    {

        // TODO update, temporary casting until signature is changed
        SidebarWidget* sideBarWidget = dynamic_cast<SidebarWidget*> (widget);

        if (sideBarWidget)
        {
            sideBarWidget->changeAlbumFromHistory(album);
            slotLeftSideBarActivate(sideBarWidget);
        }

        d->parent->enableAlbumBackwardHistory(d->useAlbumHistory && !d->albumHistory->isBackwardEmpty());
        d->parent->enableAlbumForwardHistory(d->useAlbumHistory && !d->albumHistory->isForwardEmpty());
    }
}

void DigikamView::clearHistory()
{
    d->albumHistory->clearHistory();
    d->parent->enableAlbumBackwardHistory(false);
    d->parent->enableAlbumForwardHistory(false);
}

void DigikamView::getBackwardHistory(QStringList& titles)
{
    d->albumHistory->getBackwardHistory(titles);

    for (int i = 0; i < titles.size(); ++i)
    {
        titles[i] = d->userPresentableAlbumTitle(titles[i]);
    }
}

void DigikamView::getForwardHistory(QStringList& titles)
{
    d->albumHistory->getForwardHistory(titles);

    for (int i = 0; i < titles.size(); ++i)
    {
        titles[i] = d->userPresentableAlbumTitle(titles[i]);
    }
}

QString DigikamView::DigikamViewPriv::userPresentableAlbumTitle(const QString& title)
{
    if (title == SAlbum::getTemporaryHaarTitle(DatabaseSearch::HaarSketchSearch))
    {
        return i18n("Fuzzy Sketch Search");
    }
    else if (title == SAlbum::getTemporaryHaarTitle(DatabaseSearch::HaarImageSearch))
    {
        return i18n("Fuzzy Image Search");
    }
    else if (title == SAlbum::getTemporaryTitle(DatabaseSearch::MapSearch))
    {
        return i18n("Map Search");
    }
    else if (title == SAlbum::getTemporaryTitle(DatabaseSearch::AdvancedSearch) ||
             title == SAlbum::getTemporaryTitle(DatabaseSearch::KeywordSearch))
    {
        return i18n("Last Search");
    }
    else if (title == SAlbum::getTemporaryTitle(DatabaseSearch::TimeLineSearch))
    {
        return i18n("Timeline");
    }

    return title;
}

void DigikamView::slotGotoAlbumAndItem(const ImageInfo& imageInfo)
{

    kDebug() << "going to " << imageInfo;

    emit signalNoCurrentItem();

    PAlbum* album = AlbumManager::instance()->findPAlbum(imageInfo.albumId());

    d->albumFolderSideBar->setCurrentAlbum(album);
    slotLeftSideBarActivate(d->albumFolderSideBar);

    // Set the activate item url to find in the Album View after
    // all items have be reloaded.
    d->iconView->setCurrentWhenAvailable(imageInfo.id());

    // And finally toggle album manager to handle album history and
    // reload all items.
    d->albumManager->setCurrentAlbum(album);

}

void DigikamView::slotGotoDateAndItem(const ImageInfo& imageInfo)
{
    QDate date = imageInfo.dateTime().date();

    emit signalNoCurrentItem();

    // Change to Date Album view.
    // Note, that this also opens the side bar if it is closed; this is
    // considered as a feature, because it highlights that the view was changed.
    slotLeftSideBarActivate(d->dateViewSideBar);

    // Set the activate item url to find in the Album View after
    // all items have be reloaded.
    d->iconView->setCurrentWhenAvailable(imageInfo.id());

    // Change the year and month of the iconItem (day is unused).
    d->dateViewSideBar->gotoDate(date);
}

void DigikamView::slotGotoTagAndItem(int tagID)
{
    // FIXME: Arnd: don't know yet how to get the iconItem passed through ...
    //  then we would know how to use the following ...
    //  KURL url( iconItem->imageInfo()->kurl() );
    //  url.cleanPath();

    emit signalNoCurrentItem();

    // Change to Tag Folder view.
    // Note, that this also opens the side bar if it is closed; this is
    // considered as a feature, because it highlights that the view was changed.
    slotLeftSideBarActivate(d->tagViewSideBar);

    // Set the current tag in the tag folder view.
    // TODO this slot should use a TAlbum pointer directly
    TAlbum* tag = AlbumManager::instance()->findTAlbum(tagID);

    if (tag)
    {
        d->tagViewSideBar->setCurrentAlbum(tag);
    }
    else
    {
        kError() << "Could not find a tag album for tag id " << tagID;
    }

    // Set the activate item url to find in the Tag View after
    // all items have be reloaded.
    // FIXME: see above
    // d->iconView->setAlbumItemToFind(url);
}

void DigikamView::slotSelectAlbum(const KUrl& url)
{
    PAlbum* album = d->albumManager->findPAlbum(url);

    if (!album)
    {
        kWarning() << "Unable to find album for " << url;
        return;
    }

    slotLeftSideBarActivate(d->albumFolderSideBar);
    d->albumFolderSideBar->setCurrentAlbum(album);
}

void DigikamView::slotAlbumSelected(Album* album)
{
    emit signalNoCurrentItem();

    if (!album)
    {
        d->iconView->openAlbum(0);
        d->mapView->openAlbum(0);
        emit signalAlbumSelected(false);
        emit signalTagSelected(false);
        return;
    }

    if (album->type() == Album::PHYSICAL)
    {
        emit signalAlbumSelected(true);
        emit signalTagSelected(false);
    }
    else if (album->type() == Album::TAG)
    {
        emit signalAlbumSelected(false);
        /*
                kDebug()<<"Album "<<album->title()<<" selected." ;

                // Now loop through children of the people album and check if this album is a child.
                Album* peopleAlbum = AlbumManager::instance()->findTAlbum(TagsCache::instance()->tagForPath("/People"));
                int thisAlbumId = album->id();

                QList<int> children =  peopleAlbum->childAlbumIds(true);

                foreach(int id, children)
                {
                    if(id == thisAlbumId)
                    {
                        kDebug()<<"Is a people tag";

                        showFaceAlbum(thisAlbumId);
                        emit signalTagSelected(true);
                        return;
                    }
                }
        */
        emit signalTagSelected(true);
    }
    else
    {
        emit signalAlbumSelected(false);
        emit signalTagSelected(false);
    }

    if (d->useAlbumHistory)
    {
        d->albumHistory->addAlbum(album, d->leftSideBar->getActiveTab());
    }

    d->parent->enableAlbumBackwardHistory(d->useAlbumHistory && !d->albumHistory->isBackwardEmpty());
    d->parent->enableAlbumForwardHistory(d->useAlbumHistory && !d->albumHistory->isForwardEmpty());

    d->iconView->openAlbum(album);

    if (album->isRoot())
    {
        d->stackedview->setPreviewMode(StackedView::WelcomePageMode);
    }
    else
    {
        switch (d->stackedview->previewMode())
        {
            case StackedView::PreviewImageMode:
            case StackedView::MediaPlayerMode:
            case StackedView::WelcomePageMode:
                slotTogglePreviewMode(ImageInfo());
                break;
            default: break;
        }
    }
}

/*
void DigikamView::showFaceAlbum( int tagID )
{
    QString personname = TagsCache::instance()->tagName(tagID);

    SearchXmlWriter writer;
    writer.writeGroup();
    writer.writeField ( "imagetagproperty", SearchXml::Equal );
    writer.writeValue ( QStringList() << "face" << personname );
    writer.finishField();
    writer.finishGroup();

    SAlbum* salbum = AlbumManager::instance()->createSAlbum ( personname,
                     DatabaseSearch::UnknownFaceSearch, writer.xml() );

    // search types defined in albuminfo.h. Can be a better name.
    AlbumManager::instance()->setCurrentAlbum ( salbum );
}
*/

void DigikamView::slotAlbumOpenInFileManager()
{
    Album* album = d->albumManager->currentAlbum();

    if (!album || album->type() != Album::PHYSICAL)
    {
        return;
    }

    PAlbum* palbum = dynamic_cast<PAlbum*>(album);

    new KRun(KUrl(palbum->folderPath()), this); // KRun will delete itself.
}

void DigikamView::slotAlbumOpenInTerminal()
{
    Album* album = d->albumManager->currentAlbum();

    if (!album || album->type() != Album::PHYSICAL)
    {
        return;
    }

    PAlbum* palbum = dynamic_cast<PAlbum*>(album);

    if (!palbum)
    {
        return;
    }

    QString dir(palbum->folderPath());

    // If the given directory is not local, it can still be the URL of an
    // ioslave using UDS_LOCAL_PATH which to be converted first.
    KUrl url = KIO::NetAccess::mostLocalUrl(dir, this);

    //If the URL is local after the above conversion, set the directory.
    if (url.isLocalFile())
    {
        dir = url.toLocalFile();
    }

    KToolInvocation::invokeTerminal(QString(), dir);
}

void DigikamView::slotAlbumRefresh()
{
    // force reloading of thumbnails
    LoadingCacheInterface::cleanThumbnailCache();
    Album* album = d->iconView->currentAlbum();

    // if physical album, schedule a collection scan of current album's path
    if (album && album->type() == Album::PHYSICAL)
    {
        ScanController::instance()->scheduleCollectionScan(static_cast<PAlbum*>(album)->folderPath());
    }

    // force reload. Should normally not be necessary, but we may have bugs
    qlonglong currentId = d->iconView->currentInfo().id();
    d->iconView->imageAlbumModel()->refresh();

    if (currentId != -1)
    {
        d->iconView->setCurrentWhenAvailable(currentId);
    }
}

void DigikamView::slotImageSelected()
{
    // delay to slotDispatchImageSelected
    d->needDispatchSelection = true;
    d->selectionTimer->start();
    emit signalSelectionChanged(d->iconView->numberOfSelectedIndexes());
}

void DigikamView::slotDispatchImageSelected()
{
    if (d->needDispatchSelection)
    {
        // the list of ImageInfos of currently selected items, currentItem first
        // since the iconView tracks the changes also while we are in map widget mode,
        // we can still pull the data from the iconView
        const ImageInfoList list = d->iconView->selectedImageInfosCurrentFirst();

        const ImageInfoList allImages = d->iconView->imageInfos();

        if (list.isEmpty())
        {
            d->stackedview->setPreviewItem();
            emit signalImageSelected(list, false, false, allImages);
            emit signalNoCurrentItem();
        }
        else
        {
            d->rightSideBar->itemChanged(list);

            ImageInfo previousInfo;
            ImageInfo nextInfo;

            if (d->stackedview->previewMode() != StackedView::MapWidgetMode)
            {
                previousInfo = d->iconView->previousInfo(list.first());
                nextInfo = d->iconView->nextInfo(list.first());
            }

            if (   (d->stackedview->previewMode() != StackedView::PreviewAlbumMode)
                && (d->stackedview->previewMode() != StackedView::MapWidgetMode) )
            {
                d->stackedview->setPreviewItem(list.first(), previousInfo, nextInfo);
            }

            emit signalImageSelected(list, !previousInfo.isNull(), !nextInfo.isNull(), allImages);
        }

        d->needDispatchSelection = false;
    }
}

double DigikamView::zoomMin()
{
    return d->stackedview->zoomMin();
}

double DigikamView::zoomMax()
{
    return d->stackedview->zoomMax();
}

void DigikamView::setZoomFactor(double zoom)
{
    d->stackedview->setZoomFactorSnapped(zoom);
}

void DigikamView::slotZoomFactorChanged(double zoom)
{
    toggleZoomActions();
    emit signalZoomChanged(zoom);
}

void DigikamView::setThumbSize(int size)
{
    if (d->stackedview->previewMode() == StackedView::PreviewImageMode)
    {
        double z = DZoomBar::zoomFromSize(size, zoomMin(), zoomMax());
        setZoomFactor(z);
    }
    else if (d->stackedview->previewMode() == StackedView::PreviewAlbumMode)
    {
        if (size > ThumbnailSize::Huge)
        {
            d->thumbSize = ThumbnailSize::Huge;
        }
        else if (size < ThumbnailSize::Small)
        {
            d->thumbSize = ThumbnailSize::Small;
        }
        else
        {
            d->thumbSize = size;
        }

        emit signalThumbSizeChanged(d->thumbSize);

        d->thumbSizeTimer->start();
    }
}

void DigikamView::slotThumbSizeEffect()
{
    d->iconView->setThumbnailSize(d->thumbSize);
    toggleZoomActions();

    AlbumSettings::instance()->setDefaultIconSize(d->thumbSize);
}

void DigikamView::toggleZoomActions()
{
    if (d->stackedview->previewMode() == StackedView::PreviewImageMode)
    {
        d->parent->enableZoomMinusAction(true);
        d->parent->enableZoomPlusAction(true);

        if (d->stackedview->maxZoom())
        {
            d->parent->enableZoomPlusAction(false);
        }

        if (d->stackedview->minZoom())
        {
            d->parent->enableZoomMinusAction(false);
        }
    }
    else if (d->stackedview->previewMode() == StackedView::PreviewAlbumMode)
    {
        d->parent->enableZoomMinusAction(true);
        d->parent->enableZoomPlusAction(true);

        if (d->thumbSize >= ThumbnailSize::Huge)
        {
            d->parent->enableZoomPlusAction(false);
        }

        if (d->thumbSize <= ThumbnailSize::Small)
        {
            d->parent->enableZoomMinusAction(false);
        }
    }
    else
    {
        d->parent->enableZoomMinusAction(false);
        d->parent->enableZoomPlusAction(false);
    }
}

void DigikamView::slotZoomIn()
{
    if (d->stackedview->previewMode() == StackedView::PreviewAlbumMode)
    {
        setThumbSize(d->thumbSize + ThumbnailSize::Step);
        toggleZoomActions();
        emit signalThumbSizeChanged(d->thumbSize);
    }
    else if (d->stackedview->previewMode() == StackedView::PreviewImageMode)
    {
        d->stackedview->increaseZoom();
    }
}

void DigikamView::slotZoomOut()
{
    if (d->stackedview->previewMode() == StackedView::PreviewAlbumMode)
    {
        setThumbSize(d->thumbSize - ThumbnailSize::Step);
        toggleZoomActions();
        emit signalThumbSizeChanged(d->thumbSize);
    }
    else if (d->stackedview->previewMode() == StackedView::PreviewImageMode)
    {
        d->stackedview->decreaseZoom();
    }
}

void DigikamView::slotZoomTo100Percents()
{
    if (d->stackedview->previewMode() == StackedView::PreviewImageMode)
    {
        d->stackedview->toggleFitToWindowOr100();
    }
}

void DigikamView::slotFitToWindow()
{
    if (d->stackedview->previewMode() == StackedView::PreviewImageMode)
    {
        d->stackedview->fitToWindow();
    }
}

void DigikamView::slotAlbumPropsEdit()
{
    d->albumModificationHelper->slotAlbumEdit(d->albumManager->currentPAlbum());
}

void DigikamView::connectBatchSyncMetadata(BatchSyncMetadata* syncMetadata)
{
    connect(syncMetadata, SIGNAL(signalProgressBarMode(int, const QString&)),
            d->parent, SLOT(slotProgressBarMode(int, const QString&)));

    connect(syncMetadata, SIGNAL(signalProgressValue(int)),
            d->parent, SLOT(slotProgressValue(int)));

    //connect(syncMetadata, SIGNAL(signalComplete()),
    //      this, SLOT(slotAlbumSyncPicturesMetadataDone()));

    connect(d->parent, SIGNAL(signalCancelButtonPressed()),
            syncMetadata, SLOT(slotAbort()));
}

void DigikamView::slotAlbumWriteMetadata()
{
    Album* album = d->albumManager->currentAlbum();

    if (!album)
    {
        return;
    }

    BatchSyncMetadata* syncMetadata = new BatchSyncMetadata(album, BatchSyncMetadata::WriteFromDatabaseToFile, this);
    connectBatchSyncMetadata(syncMetadata);
    syncMetadata->parseAlbum();
}

void DigikamView::slotAlbumReadMetadata()
{
    Album* album = d->albumManager->currentAlbum();

    if (!album)
    {
        return;
    }

    BatchSyncMetadata* syncMetadata = new BatchSyncMetadata(album, BatchSyncMetadata::ReadFromFileToDatabase, this);
    connectBatchSyncMetadata(syncMetadata);
    syncMetadata->parseAlbum();
}

void DigikamView::slotImageWriteMetadata()
{
    ImageInfoList selected = d->iconView->selectedImageInfos();

    BatchSyncMetadata* syncMetadata = new BatchSyncMetadata(selected, BatchSyncMetadata::WriteFromDatabaseToFile, this);
    connectBatchSyncMetadata(syncMetadata);
    syncMetadata->parseList();
}

void DigikamView::slotImageReadMetadata()
{
    ImageInfoList selected = d->iconView->selectedImageInfos();

    BatchSyncMetadata* syncMetadata = new BatchSyncMetadata(selected, BatchSyncMetadata::ReadFromFileToDatabase, this);
    connectBatchSyncMetadata(syncMetadata);
    syncMetadata->parseList();
}

// ----------------------------------------------------------------

void DigikamView::slotEscapePreview()
{
    if (d->stackedview->previewMode() == StackedView::PreviewAlbumMode ||
        d->stackedview->previewMode() == StackedView::WelcomePageMode)
    {
        return;
    }

    // pass a null image info, because we want to fall back to the old
    // view mode
    slotTogglePreviewMode(ImageInfo());
}

void DigikamView::slotMapWidgetView()
{
    d->stackedview->setPreviewMode(StackedView::MapWidgetMode);
}

void DigikamView::slotIconView()
{
    if (d->stackedview->previewMode() == StackedView::PreviewImageMode)
    {
        emit signalThumbSizeChanged(d->iconView->thumbnailSize().size());
    }

    // and switch to icon view
    d->stackedview->setPreviewMode(StackedView::PreviewAlbumMode);

    // make sure the next/previous buttons are updated
    slotImageSelected();
}

void DigikamView::slotImagePreview()
{
    const int currentPreviewMode = d->stackedview->previewMode();
    ImageInfo currentInfo;
    if (currentPreviewMode == StackedView::PreviewAlbumMode)
    {
        currentInfo = d->iconView->currentInfo();
    }
    else if (currentPreviewMode == StackedView::MapWidgetMode)
    {
        currentInfo = d->mapView->currentInfo();
    }

    slotTogglePreviewMode(currentInfo);
}

/**
 * @brief This method toggles between AlbumView/MapWidgetView and ImagePreview modes, depending on the context.
 */
void DigikamView::slotTogglePreviewMode(const ImageInfo& info)
{
    if (  (d->stackedview->previewMode() == StackedView::PreviewAlbumMode
           || d->stackedview->previewMode() == StackedView::MapWidgetMode)
        && !info.isNull() )
    {
        d->lastPreviewMode = d->stackedview->previewMode();

        if (d->stackedview->previewMode() == StackedView::PreviewAlbumMode)
        {
            d->stackedview->setPreviewItem(info, d->iconView->previousInfo(info), d->iconView->nextInfo(info));
        }
        else
        {
            d->stackedview->setPreviewItem(info, ImageInfo(), ImageInfo());
        }
    }
    else
    {
        // go back to either AlbumViewMode or MapWidgetMode
        d->stackedview->setPreviewMode( d->lastPreviewMode );
    }

    // make sure the next/previous buttons are updated
    slotImageSelected();
}

void DigikamView::slotViewModeChanged()
{
    toggleZoomActions();

    switch (d->stackedview->previewMode())
    {
        case StackedView::PreviewAlbumMode:
            emit signalSwitchedToIconView();
            emit signalThumbSizeChanged(d->iconView->thumbnailSize().size());
            break;
        case StackedView::PreviewImageMode:
            emit signalSwitchedToPreview();
            slotZoomFactorChanged(d->stackedview->zoomFactor());
            break;
        case StackedView::WelcomePageMode:
            emit signalSwitchedToIconView();
            break;
        case StackedView::MediaPlayerMode:
            emit signalSwitchedToPreview();
            break;
        case StackedView::MapWidgetMode:
            emit signalSwitchedToMapView();
            //TODO: connect map view's zoom buttons to main status bar zoom buttons
            break;
    }
}

void DigikamView::slotImageFindSimilar()
{
    ImageInfo current = d->iconView->currentInfo();

    if (!current.isNull())
    {
        d->fuzzySearchSideBar->newSimilarSearch(current);
        slotLeftSideBarActivate(d->fuzzySearchSideBar);
    }
}

void DigikamView::slotImageExifOrientation(int orientation)
{
    d->iconView->setExifOrientationOfSelected(orientation);
}

void DigikamView::slotEditor()
{
    d->iconView->openEditor();
}

void DigikamView::slotLightTable()
{
    d->iconView->setOnLightTable();
}

void DigikamView::slotQueueMgr()
{
    d->iconView->insertToQueue();
}

void DigikamView::slotImageEdit()
{
    d->iconView->openCurrentInEditor();
}

void DigikamView::slotImageLightTable()
{
    /// @todo Care about MapWidgetMode
    if (d->stackedview->previewMode() == StackedView::PreviewAlbumMode)
    {
        // put images into an emptied light table
        d->iconView->setSelectedOnLightTable();
    }
    else
    {
        ImageInfoList list;
        ImageInfo info = d->stackedview->imagePreviewView()->getImageInfo();
        list.append(info);
        // put images into an emptied light table
        d->iconView->utilities()->insertToLightTable(list, info, false);
    }
}

void DigikamView::slotImageAddToLightTable()
{
    /// @todo Care about MapWidgetMode
    if (d->stackedview->previewMode() == StackedView::PreviewAlbumMode)
    {
        // add images to the existing images in the light table
        d->iconView->addSelectedToLightTable();
    }
    else
    {
        ImageInfoList list;
        ImageInfo info = d->stackedview->imagePreviewView()->getImageInfo();
        list.append(info);
        // add images to the existing images in the light table
        d->iconView->utilities()->insertToLightTable(list, info, true);
    }
}

void DigikamView::slotImageAddToCurrentQueue()
{
    /// @todo Care about MapWidgetMode
    if (d->stackedview->previewMode() == StackedView::PreviewAlbumMode)
    {
        d->iconView->insertSelectedToCurrentQueue();
    }
    else
    {
        ImageInfoList list;
        ImageInfo info = d->stackedview->imagePreviewView()->getImageInfo();
        list.append(info);
        d->iconView->utilities()->insertToQueueManager(list, info, false);
    }
}

void DigikamView::slotImageAddToNewQueue()
{
    /// @todo Care about MapWidgetMode
    bool newQueue = QueueMgrWindow::queueManagerWindowCreated() &&
                    !QueueMgrWindow::queueManagerWindow()->queuesMap().isEmpty();

    if (d->stackedview->previewMode() == StackedView::PreviewAlbumMode)
    {
        if (newQueue)
        {
            d->iconView->insertSelectedToNewQueue();
        }
        else
        {
            d->iconView->insertSelectedToCurrentQueue();
        }
    }
    else
    {
        // FIXME
        ImageInfoList list;
        ImageInfo info = d->stackedview->imagePreviewView()->getImageInfo();
        list.append(info);
        d->iconView->utilities()->insertToQueueManager(list, info, newQueue);
    }
}

void DigikamView::slotImageAddToExistingQueue(int queueid)
{
    ImageInfoList list;

    if (d->stackedview->previewMode() == StackedView::PreviewAlbumMode)
    {
        list = d->stackedview->imageIconView()->selectedImageInfos();
    }
    else
    {
        list << d->stackedview->imagePreviewView()->getImageInfo();
    }

    if (!list.isEmpty())
    {
        d->iconView->utilities()->insertSilentToQueueManager(list, list.first(), queueid);
    }
}

void DigikamView::slotImageRename()
{
    d->iconView->rename();
}

void DigikamView::slotImageDelete()
{
    d->iconView->deleteSelected(false);
}

void DigikamView::slotImageDeletePermanently()
{
    d->iconView->deleteSelected(true);
}

void DigikamView::slotImageDeletePermanentlyDirectly()
{
    d->iconView->deleteSelectedDirectly(false);
}

void DigikamView::slotImageTrashDirectly()
{
    d->iconView->deleteSelectedDirectly(true);
}

void DigikamView::slotSelectAll()
{
    d->iconView->selectAll();
}

void DigikamView::slotSelectNone()
{
    d->iconView->clearSelection();
}

void DigikamView::slotSelectInvert()
{
    d->iconView->invertSelection();
}

void DigikamView::slotSortImages(int sortRole)
{
    AlbumSettings* settings = AlbumSettings::instance();

    if (!settings)
    {
        return;
    }

    settings->setImageSortOrder(sortRole);
    d->iconView->imageFilterModel()->setSortRole((ImageSortSettings::SortRole) sortRole);
}

void DigikamView::slotSortImagesOrder(int order)
{
    AlbumSettings* settings = AlbumSettings::instance();

    if (!settings)
    {
        return;
    }

    settings->setImageSorting(order);
    d->iconView->imageFilterModel()->setSortOrder((ImageSortSettings::SortOrder) order);
}

void DigikamView::slotGroupImages(int categoryMode)
{
    AlbumSettings* settings = AlbumSettings::instance();

    if (!settings)
    {
        return;
    }

    settings->setImageGroupMode(categoryMode);
    d->iconView->imageFilterModel()->setCategorizationMode((ImageSortSettings::CategorizationMode) categoryMode);
}

void DigikamView::slotMoveSelectionToAlbum()
{
    d->iconView->createNewAlbumForSelected();
}

void DigikamView::slotLeftSidebarChangedTab(QWidget* w)
{
    // TODO update, temporary cast
    SidebarWidget* widget = dynamic_cast<SidebarWidget*> (w);
    foreach (SidebarWidget* sideBarWidget, d->leftSideBarWidgets)
    {
        bool active = (widget && (widget == sideBarWidget));
        sideBarWidget->setActive(active);
    }
}

void DigikamView::toggleTag(int tagID)
{
    d->iconView->toggleTagToSelected(tagID);
}

void DigikamView::slotAssignPickLabel(int pickId)
{
    d->iconView->assignPickLabelToSelected(pickId);
}

void DigikamView::slotAssignColorLabel(int colorId)
{
    d->iconView->assignColorLabelToSelected(colorId);
}

void DigikamView::slotAssignRating(int rating)
{
    d->iconView->assignRatingToSelected(rating);
}

void DigikamView::slotSlideShowAll()
{
    slideShow(d->iconView->imageInfos());
}

void DigikamView::slotSlideShowSelection()
{
    slideShow(d->iconView->selectedImageInfos());
}

void DigikamView::slotSlideShowRecursive()
{
    Album* album = AlbumManager::instance()->currentAlbum();

    if (album)
    {
        AlbumList albumList;
        albumList.append(album);
        AlbumIterator it(album);

        while (it.current())
        {
            albumList.append(*it);
            ++it;
        }

        ImageInfoAlbumsJob* job = new ImageInfoAlbumsJob;
        connect(job, SIGNAL(signalCompleted(const ImageInfoList&)),
                this, SLOT(slotItemsInfoFromAlbums(const ImageInfoList&)));
        job->allItemsFromAlbums(albumList);
    }
}

void DigikamView::slotItemsInfoFromAlbums(const ImageInfoList& infoList)
{
    ImageInfoList list = infoList;
    slideShow(list);
}

void DigikamView::slideShow(const ImageInfoList& infoList)
{
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup group        = config->group("ImageViewer Settings");
    bool startWithCurrent     = group.readEntry("SlideShowStartCurrent", false);

    int     i = 0;
    float cnt = (float)infoList.count();
    emit signalProgressBarMode(StatusProgressBar::CancelProgressBarMode,
                               i18np("Preparing slideshow of 1 image. Please wait...","Preparing slideshow of %1 images. Please wait...", infoList.count()));

    SlideShowSettings settings;
    settings.exifRotate           = MetadataSettings::instance()->settings().exifRotate;
    settings.delay                = group.readEntry("SlideShowDelay", 5) * 1000;
    settings.printName            = group.readEntry("SlideShowPrintName", true);
    settings.printDate            = group.readEntry("SlideShowPrintDate", false);
    settings.printApertureFocal   = group.readEntry("SlideShowPrintApertureFocal", false);
    settings.printExpoSensitivity = group.readEntry("SlideShowPrintExpoSensitivity", false);
    settings.printMakeModel       = group.readEntry("SlideShowPrintMakeModel", false);
    settings.printComment         = group.readEntry("SlideShowPrintComment", false);
    settings.printLabels          = group.readEntry("SlideShowPrintLabels", false);
    settings.loop                 = group.readEntry("SlideShowLoop", false);

    d->cancelSlideShow = false;

    for (ImageInfoList::const_iterator it = infoList.constBegin();
         !d->cancelSlideShow && (it != infoList.constEnd()) ; ++it)
    {
        ImageInfo info = *it;
        settings.fileList.append(info.fileUrl());
        SlidePictureInfo pictInfo;
        pictInfo.comment    = info.comment();
        pictInfo.rating     = info.rating();
        pictInfo.colorLabel = info.colorLabel();
        pictInfo.pickLabel  = info.pickLabel();
        pictInfo.photoInfo  = info.photoInfoContainer();
        settings.pictInfoMap.insert(info.fileUrl(), pictInfo);

        emit signalProgressValue((int)((i++/cnt)*100.0));
        kapp->processEvents();
    }

    emit signalProgressBarMode(StatusProgressBar::TextMode, QString());

    if (!d->cancelSlideShow)
    {
        SlideShow* slide = new SlideShow(settings);

        if (startWithCurrent)
        {
            slide->setCurrent(d->iconView->currentUrl());
        }

        connect(slide, SIGNAL(signalRatingChanged(const KUrl&, int)),
                this, SLOT(slotRatingChanged(const KUrl&, int)));

        connect(slide, SIGNAL(signalColorLabelChanged(const KUrl&, int)),
                this, SLOT(slotColorLabelChanged(const KUrl&, int)));

        connect(slide, SIGNAL(signalPickLabelChanged(const KUrl&, int)),
                this, SLOT(slotPickLabelChanged(const KUrl&, int)));

        slide->show();
    }
}

void DigikamView::slotCancelSlideShow()
{
    d->cancelSlideShow = true;
}

void DigikamView::toggleShowBar(bool b)
{
    d->stackedview->thumbBarDock()->showThumbBar(b);
}

void DigikamView::setRecurseAlbums(bool recursive)
{
    d->iconView->imageAlbumModel()->setRecurseAlbums(recursive);
}

void DigikamView::setRecurseTags(bool recursive)
{
    d->iconView->imageAlbumModel()->setRecurseTags(recursive);
}

void DigikamView::slotSidebarTabTitleStyleChanged()
{
    d->leftSideBar->setStyle(AlbumSettings::instance()->getSidebarTitleStyle());
    d->rightSideBar->setStyle(AlbumSettings::instance()->getSidebarTitleStyle());

    /// @todo Which settings actually have to be reloaded?
//     d->rightSideBar->applySettings();
}

void DigikamView::slotProgressMessageChanged(const QString& descriptionOfAction)
{
    emit signalProgressBarMode(StatusProgressBar::ProgressBarMode, descriptionOfAction);
}

void DigikamView::slotProgressValueChanged(float percent)
{
    emit signalProgressValue(lround(percent * 100));
}

void DigikamView::slotProgressFinished()
{
    emit signalProgressBarMode(StatusProgressBar::TextMode, QString());
}

void DigikamView::slotOrientationChangeFailed(const QStringList& failedFileNames)
{
    if (failedFileNames.isEmpty())
    {
        return;
    }

    if (failedFileNames.count() == 1)
    {
        KMessageBox::error(0, i18n("Failed to revise Exif orientation for file %1.",
                                   failedFileNames[0]));
    }
    else
    {
        KMessageBox::errorList(0, i18n("Failed to revise Exif orientation these files:"),
                               failedFileNames);
    }
}

void DigikamView::slotLeftSideBarActivateAlbums()
{
    d->leftSideBar->setActiveTab(d->albumFolderSideBar);
}

void DigikamView::slotLeftSideBarActivateTags()
{
    d->leftSideBar->setActiveTab(d->tagViewSideBar);
}

void DigikamView::slotLeftSideBarActivate(SidebarWidget* widget)
{
    d->leftSideBar->setActiveTab(widget);
}

void DigikamView::slotLeftSideBarActivate(QWidget* widget)
{
    slotLeftSideBarActivate(static_cast<SidebarWidget*>(widget));
}

void DigikamView::slotRatingChanged(const KUrl& url, int rating)
{
    rating = qMin(RatingMax, qMax(RatingMin, rating));
    ImageInfo info(url);

    if (!info.isNull())
    {
        MetadataHub hub;
        hub.load(info);
        hub.setRating(rating);
        hub.write(info, MetadataHub::PartialWrite);
        hub.write(info.filePath(), MetadataHub::FullWriteIfChanged);
    }
}

void DigikamView::slotColorLabelChanged(const KUrl& url, int color)
{
    ImageInfo info(url);

    if (!info.isNull())
    {
        MetadataHub hub;
        hub.load(info);
        hub.setColorLabel(color);
        hub.write(info, MetadataHub::PartialWrite);
        hub.write(info.filePath(), MetadataHub::FullWriteIfChanged);
    }
}

void DigikamView::slotPickLabelChanged(const KUrl& url, int pick)
{
    ImageInfo info(url);

    if (!info.isNull())
    {
        MetadataHub hub;
        hub.load(info);
        hub.setPickLabel(pick);
        hub.write(info, MetadataHub::PartialWrite);
        hub.write(info.filePath(), MetadataHub::FullWriteIfChanged);
    }
}

bool DigikamView::hasCurrentItem() const
{
    // We should actually get this directly from the selection model,
    // but the iconView is fine for now.
    return !d->iconView->currentInfo().isNull();
}

}  // namespace Digikam
