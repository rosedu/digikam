/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2008-05-30
 * Description : GPS search sidebar tab contents.
 *
 * Copyright (C) 2008-2010 by Gilles Caulier <caulier dot gilles at gmail dot com>
 * Copyright (C) 2009 by Johannes Wienke <languitar at semipol dot de>
 * Copyright (C) 2010, 2011 by Michael G. Hansen <mike at mghansen dot de>
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

#include "gpssearchview.moc"

// Qt includes

#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QSplitter>
#include <QToolButton>
#include <QTimer>
#include <QMenu>
#include <QActionGroup>
#include <QAction>

// KDE includes

#include <kaction.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdialog.h>
#include <khbox.h>
#include <kinputdialog.h>

// Local includes

#include "editablesearchtreeview.h"
#include "imageinfojob.h"
#include "searchxml.h"
#include "gpsmarkertiler.h"
#include "digikam2kmap.h"

namespace Digikam
{

class GPSSearchView::GPSSearchViewPriv
{

public:

    GPSSearchViewPriv() :
        saveBtn(0),
        nameEdit(0),
        imageInfoJob(),
        searchGPSBar(0),
        searchTreeView(0),
        splitter(0),
        mapSearchWidget(0),
        gpsMarkerTiler(0),
        imageAlbumModel(0),
        imageFilterModel(0),
        selectionModel(0),
        searchModel(0),
        sortOrderOptionsHelper(0)
    {
    }

    static const QString        configSplitterStateEntry;
    QToolButton*                saveBtn;
    KLineEdit*                  nameEdit;
    ImageInfoJob                imageInfoJob;
    SearchTextBar*              searchGPSBar;
    EditableSearchTreeView*     searchTreeView;
    QSplitter*                  splitter;
    KMap::KMapWidget*           mapSearchWidget;
    GPSMarkerTiler*             gpsMarkerTiler;
    ImageAlbumModel*            imageAlbumModel;
    ImageFilterModel*           imageFilterModel;
    QItemSelectionModel*        selectionModel;
    SearchModel*                searchModel;
    GPSImageInfoSorter*         sortOrderOptionsHelper;
};
const QString GPSSearchView::GPSSearchViewPriv::configSplitterStateEntry("SplitterState");

/**
 * @brief Constructor
 * @param parent Parent object.
 * @param searchModel The model that stores the searches.
 * @param imageFilterModel The image model used by displaying the selected images on map.
 * @param itemSelectionModel The selection model corresponding to the imageFilterModel.
 */
GPSSearchView::GPSSearchView(QWidget* parent, SearchModel* searchModel,
                             SearchModificationHelper* searchModificationHelper,
                             ImageFilterModel* imageFilterModel, QItemSelectionModel* itemSelectionModel)
    : QWidget(parent), StateSavingObject(this),
      d(new GPSSearchViewPriv)
{
    setAttribute(Qt::WA_DeleteOnClose);

    /// @todo Really?
    setAcceptDrops(true);

    d->imageAlbumModel       = qobject_cast<ImageAlbumModel*>(imageFilterModel->sourceModel());
    d->selectionModel        = itemSelectionModel;
    d->imageFilterModel      = imageFilterModel;
    d->searchModel           = searchModel;

    // ---------------------------------------------------------------

    QVBoxLayout* vlay  = new QVBoxLayout(this);

    QFrame* mapPanel   = new QFrame(this);
    mapPanel->setMinimumWidth(256);
    mapPanel->setMinimumHeight(256);
    QVBoxLayout* vlay2 = new QVBoxLayout(mapPanel);
    d->mapSearchWidget = new KMap::KMapWidget(mapPanel);
    d->mapSearchWidget->setBackend("marble");
    d->mapSearchWidget->setShowThumbnails(true);

    d->gpsMarkerTiler = new GPSMarkerTiler(this, d->imageFilterModel, d->selectionModel);
    d->mapSearchWidget->setGroupedModel(d->gpsMarkerTiler);

    mapPanel->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    mapPanel->setLineWidth(style()->pixelMetric(QStyle::PM_DefaultFrameWidth));

    d->sortOrderOptionsHelper = new GPSImageInfoSorter(this);
    d->sortOrderOptionsHelper->addToKMapWidget(d->mapSearchWidget);

    vlay2->addWidget(d->mapSearchWidget);
    vlay2->setMargin(0);
    vlay2->setSpacing(0);

    // ---------------------------------------------------------------

    KHBox* hbox = new KHBox(this);
    hbox->setMargin(0);
    hbox->setSpacing(KDialog::spacingHint());

    d->nameEdit = new KLineEdit(hbox);
    d->nameEdit->setClearButtonShown(true);
    d->nameEdit->setWhatsThis(i18n("Enter the name of the current map search to save in the "
                                   "\"My Map Searches\" view."));

    d->saveBtn  = new QToolButton(hbox);
    d->saveBtn->setIcon(SmallIcon("document-save"));
    d->saveBtn->setEnabled(false);
    d->saveBtn->setToolTip(i18n("Save current map search to a new virtual album."));
    d->saveBtn->setWhatsThis(i18n("If this button is pressed, the current map search "
                                  "will be saved to a new search "
                                  "virtual album using the name "
                                  "set on the left side."));

    // ---------------------------------------------------------------

    d->searchTreeView = new EditableSearchTreeView(this, d->searchModel, searchModificationHelper);
    d->searchTreeView->filteredModel()->setFilterSearchType(DatabaseSearch::MapSearch);
    d->searchTreeView->filteredModel()->setListTemporarySearches(true);
    d->searchTreeView->setAlbumManagerCurrentAlbum(true);
    d->searchGPSBar   = new SearchTextBar(this, "GPSSearchViewSearchGPSBar");
    d->searchGPSBar->setModel(d->searchTreeView->filteredModel(), AbstractAlbumModel::AlbumIdRole, AbstractAlbumModel::AlbumTitleRole);
    d->searchGPSBar->setFilterModel(d->searchTreeView->albumFilterModel());

    // ---------------------------------------------------------------

    d->splitter = new QSplitter(Qt::Vertical, this);
    d->splitter->setOpaqueResize(KGlobalSettings::opaqueResize());

    QFrame* const frameTop     = new QFrame(d->splitter);
    QVBoxLayout* const vlayTop = new QVBoxLayout(frameTop);
    vlayTop->addWidget(mapPanel);
    vlayTop->addWidget(d->mapSearchWidget->getControlWidget());
    d->mapSearchWidget->setAvailableMouseModes(
                KMap::MouseModePan |
                KMap::MouseModeRegionSelection |
                KMap::MouseModeZoomIntoGroup |
                KMap::MouseModeRegionSelectionFromIcon |
                KMap::MouseModeFilter |
                KMap::MouseModeSelectThumbnail
            );
    d->mapSearchWidget->setVisibleMouseModes(
                KMap::MouseModePan |
                KMap::MouseModeZoomIntoGroup |
                KMap::MouseModeFilter |
                KMap::MouseModeSelectThumbnail
            );

    // construct a second row of control actions below the control widget
    /// @todo Should we still replace the icons of the actions with text as discussed during the sprint?
    QWidget* const secondActionRow = new QWidget();
    QHBoxLayout* const secondActionRowHBox = new QHBoxLayout();
    secondActionRowHBox->setMargin(0);
    secondActionRow->setLayout(secondActionRowHBox);

    QLabel* const secondActionRowLabel = new QLabel(i18n("Search by area:"));
    secondActionRowHBox->addWidget(secondActionRowLabel);

    QToolButton* const tbRegionSelection = new QToolButton(secondActionRow);
    tbRegionSelection->setDefaultAction(d->mapSearchWidget->getControlAction("mousemode-regionselectionmode"));
    secondActionRowHBox->addWidget(tbRegionSelection);

    QToolButton* const tbRegionFromIcon = new QToolButton(secondActionRow);
    tbRegionFromIcon->setDefaultAction(d->mapSearchWidget->getControlAction("mousemode-regionselectionfromiconmode"));
    secondActionRowHBox->addWidget(tbRegionFromIcon);

    QToolButton* const tbClearRegionSelection = new QToolButton(secondActionRow);
    tbClearRegionSelection->setDefaultAction(d->mapSearchWidget->getControlAction("mousemode-removecurrentregionselection"));
    secondActionRowHBox->addWidget(tbClearRegionSelection);

    secondActionRowHBox->addStretch(10);
    vlayTop->addWidget(secondActionRow);

    // end of the second action row

    vlayTop->addWidget(hbox);
    vlayTop->setStretchFactor(mapPanel, 10);
    vlayTop->setMargin(0);
    vlayTop->setSpacing(KDialog::spacingHint());
    QFrame* const frameBottom     = new QFrame(d->splitter);
    QVBoxLayout* const vlayBottom = new QVBoxLayout(frameBottom);
    vlayBottom->addWidget(d->searchTreeView);
    vlayBottom->addWidget(d->searchGPSBar);
    vlayBottom->setMargin(0);
    vlayBottom->setSpacing(KDialog::spacingHint());

    d->splitter->addWidget(frameTop);
    d->splitter->addWidget(frameBottom);

    // ---------------------------------------------------------------

    vlay->addWidget(d->splitter);

    // ---------------------------------------------------------------

    connect(d->searchTreeView, SIGNAL(currentAlbumChanged(Album*)),
            this, SLOT(slotAlbumSelected(Album*)));

    connect(d->saveBtn, SIGNAL(clicked()),
            this, SLOT(slotSaveGPSSAlbum()));

    connect(d->nameEdit, SIGNAL(textChanged(const QString&)),
            this, SLOT(slotCheckNameEditGPSConditions()));

    connect(d->nameEdit, SIGNAL(returnPressed(const QString&)),
            d->saveBtn, SLOT(animateClick()));

    connect(d->mapSearchWidget, SIGNAL(signalRegionSelectionChanged()),
            this, SLOT(slotRegionSelectionChanged()));

    connect(d->gpsMarkerTiler, SIGNAL(signalModelFilteredImages(const QList<qlonglong>&)),
            this, SLOT(slotMapSoloItems(const QList<qlonglong>&)));

    connect(d->mapSearchWidget, SIGNAL(signalRemoveCurrentFilter()),
            this, SLOT(slotRemoveCurrentFilter()));

    // ---------------------------------------------------------------

    slotCheckNameEditGPSConditions();
}

GPSSearchView::~GPSSearchView()
{
    delete d;
}

void GPSSearchView::setConfigGroup(KConfigGroup group)
{
    StateSavingObject::setConfigGroup(group);
    d->searchTreeView->setConfigGroup(group);
}

void GPSSearchView::doLoadState()
{
    KConfigGroup group = getConfigGroup();

    if (group.hasKey(entryName(d->configSplitterStateEntry)))
    {
        const QByteArray splitterState = QByteArray::fromBase64(group.readEntry(entryName(d->configSplitterStateEntry), QByteArray()));

        if (!splitterState.isEmpty())
        {
            d->splitter->restoreState(splitterState);
        }
    }

    d->sortOrderOptionsHelper->setSortOptions(
            GPSImageInfoSorter::SortOptions(group.readEntry(entryName("Sort Order"), int(d->sortOrderOptionsHelper->getSortOptions())))
        );

    const KConfigGroup groupMapWidget = KConfigGroup(&group, entryName("GPSSearch Map Widget"));

    d->mapSearchWidget->readSettingsFromGroup(&groupMapWidget);

    d->searchTreeView->loadState();

    AlbumManager::instance()->setCurrentAlbum(0);

    d->searchTreeView->clearSelection();
}

void GPSSearchView::doSaveState()
{
    KConfigGroup group = getConfigGroup();

    group.writeEntry(entryName(d->configSplitterStateEntry), d->splitter->saveState().toBase64());
    group.writeEntry(entryName("Sort Order"), int(d->sortOrderOptionsHelper->getSortOptions()));

    KConfigGroup groupMapWidget = KConfigGroup(&group, entryName("GPSSearch Map Widget"));
    d->mapSearchWidget->saveSettingsToGroup(&groupMapWidget);
    d->searchTreeView->saveState();

    group.sync();
}

/**
 * @brief Sets the widget active or inactive.
 *
 * Called when the GPSSearch tab becomes the current/not current tab.
 *
 * @param state When true, the widget is enabled.
 */
void GPSSearchView::setActive(bool state)
{
    if (!state)
    {
        // make sure we reset the custom filters set by the map:
        emit(signalMapSoloItems(QList<qlonglong>(), "gpssearch"));
        d->mapSearchWidget->setActive(false);
    }
    else
    {
        d->mapSearchWidget->setActive(true);

        if (d->searchTreeView->currentAlbum())
        {
           AlbumManager::instance()->setCurrentAlbum(
                           d->searchTreeView->currentAlbum());
        }

        slotClearImages();
    }
}

void GPSSearchView::changeAlbumFromHistory(SAlbum* album)
{
    d->searchTreeView->setCurrentAlbum(album);
}

/**
 * This slot saves the current album.
 */
void GPSSearchView::slotSaveGPSSAlbum()
{
    QString name = d->nameEdit->text();

    if (!checkName(name))
    {
        return;
    }

    createNewGPSSearchAlbum(name);
}

/**
 * This slot is called when a new selection is made. It creates a new Search Album.
 */
void GPSSearchView::slotRegionSelectionChanged()
{
    const KMap::GeoCoordinates::Pair newRegionSelection = d->mapSearchWidget->getRegionSelection();
    const bool haveRegionSelection = newRegionSelection.first.hasCoordinates();

    if (haveRegionSelection)
    {
        slotCheckNameEditGPSConditions();
        createNewGPSSearchAlbum(SAlbum::getTemporaryTitle(DatabaseSearch::MapSearch));
    }
    else
    {
        // reset the search rectangle of the temporary album:
        createNewGPSSearchAlbum(SAlbum::getTemporaryTitle(DatabaseSearch::MapSearch));
        d->gpsMarkerTiler->removeCurrentRegionSelection();
        d->searchTreeView->clearSelection();
        slotClearImages();
    }

    slotRefreshMap();
}

/**
 * @brief This function creates a new Search Album.
 * @param name The name of the new album.
 */
void GPSSearchView::createNewGPSSearchAlbum(const QString& name)
{
    //AlbumManager::instance()->setCurrentAlbum(0);

    // We query the database here

    const KMap::GeoCoordinates::Pair coordinates = d->mapSearchWidget->getRegionSelection();
    const bool haveCoordinates = coordinates.first.hasCoordinates();

    if (haveCoordinates)
    {
        d->gpsMarkerTiler->setRegionSelection(coordinates);
    }

    // NOTE: coordinates as lon1, lat1, lon2, lat2 (or West, North, East, South)
    // as left/top, right/bottom rectangle.
    QList<qreal> coordinatesList = QList<qreal>() <<
                                         coordinates.first.lon() << coordinates.first.lat() <<
                                         coordinates.second.lon() << coordinates.second.lat();

    if (!haveCoordinates)
    {
        /// @todo We need to create a search album with invalid coordinates
        coordinatesList.clear();
        coordinatesList << -200 << -200 << -200 << -200;
    }

    kDebug() << "West, North, East, South: " << coordinatesList;

    SearchXmlWriter writer;
    writer.writeGroup();
    writer.writeField("position", SearchXml::Inside);
    writer.writeAttribute("type", "rectangle");
    writer.writeValue(coordinatesList);
    writer.finishField();
    writer.finishGroup();

    SAlbum* const salbum = AlbumManager::instance()->createSAlbum(name, DatabaseSearch::MapSearch, writer.xml());
    AlbumManager::instance()->setCurrentAlbum(salbum);
    d->imageInfoJob.allItemsFromAlbum(salbum);
    d->searchTreeView->setCurrentAlbum(salbum);
    d->imageAlbumModel->openAlbum(salbum);
}

/**
 * @brief An album is selected in the saved searches list.
 * @param a This album will be selected.
 */
void GPSSearchView::slotAlbumSelected(Album* a)
{
    /// @todo This re-sets the region selection unwantedly...

    SAlbum* const salbum = dynamic_cast<SAlbum*> (a);

    if (!salbum)
    {
        return;
    }

    SearchXmlReader reader(salbum->query());
    reader.readToFirstField();
    QStringRef type = reader.attributes().value("type");

    if (type == "rectangle")
    {
        const QList<double> list = reader.valueToDoubleList();

        const KMap::GeoCoordinates::Pair coordinates(
                KMap::GeoCoordinates(list.at(1), list.at(0)),
                KMap::GeoCoordinates(list.at(3), list.at(2))
            );

        /// @todo Currently, invalid coordinates are stored as -200:
        if (list.at(1)!=-200)
        {
            d->mapSearchWidget->setRegionSelection(coordinates);
            d->gpsMarkerTiler->setRegionSelection(coordinates);
        }
        else
        {
            d->mapSearchWidget->clearRegionSelection();
            d->gpsMarkerTiler->removeCurrentRegionSelection();
        }
        slotCheckNameEditGPSConditions();
    }

    d->imageInfoJob.allItemsFromAlbum(salbum);
}

/**
 * @brief Checks whether the newly added search name already exists.
 * @param name The name of the current search.
 */
bool GPSSearchView::checkName(QString& name)
{
    bool checked = checkAlbum(name);

    while (!checked)
    {
        QString label = i18n( "Search name already exists.\n"
                              "Please enter a new name:" );
        bool ok;
        QString newTitle = KInputDialog::getText(i18n("Name exists"), label, name, &ok, this);

        if (!ok)
        {
            return false;
        }

        name    = newTitle;
        checked = checkAlbum(name);
    }

    return true;
}

/**
 * @brief Checks whether the newly added album name already exists.
 * @param name The name of the album.
 */
bool GPSSearchView::checkAlbum(const QString& name) const
{
    const AlbumList list = AlbumManager::instance()->allSAlbums();

    for (AlbumList::ConstIterator it = list.constBegin() ; it != list.constEnd() ; ++it)
    {
        const SAlbum* const album = (SAlbum*)(*it);

        if ( album->title() == name )
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Remove the current filter.
 */
void GPSSearchView::slotRemoveCurrentFilter()
{
    d->gpsMarkerTiler->setPositiveFilterIsActive(false);
    const QList<qlonglong> emptyIdList;
    emit signalMapSoloItems(emptyIdList, "gpssearch");
    slotRefreshMap();
}

/**
 * @brief Enable or disable the album saving controls.
 */
void GPSSearchView::slotCheckNameEditGPSConditions()
{
    if (d->mapSearchWidget->getRegionSelection().first.hasCoordinates())
    {
        d->nameEdit->setEnabled(true);

        if (!d->nameEdit->text().isEmpty())
        {
            d->saveBtn->setEnabled(true);
        }
    }
    else
    {
        d->nameEdit->setEnabled(false);
        d->saveBtn->setEnabled(false);
    }
}

/**
 * @brief Slot which gets called when the user makes items 'solo' on the map
 * @param gpsList List of GPSInfos which are 'solo'
 */
void GPSSearchView::slotMapSoloItems(const QList<qlonglong>& idList)
{
    emit(signalMapSoloItems(idList, "gpssearch"));
}


void GPSSearchView::slotRefreshMap()
{
    d->mapSearchWidget->refreshMap();
}

void GPSSearchView::slotClearImages()
{
    if (d->mapSearchWidget->getActiveState())
    {
        d->imageAlbumModel->clearImageInfos();
    }
}

}  // namespace Digikam
