/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2009-03-24
 * Description : Qt Model for Albums - filter model
 *
 * Copyright (C) 2008-2009 by Marcel Wiesweg <marcel dot wiesweg at gmx dot de>
 * Copyright (C) 2009 by Johannes Wienke <languitar at semipol dot de>
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

#ifndef ALBUMFILTERMODEL_H
#define ALBUMFILTERMODEL_H

// Qt includes

#include <QPointer>
#include <QSortFilterProxyModel>

// Local includes

#include "albummodel.h"
#include "searchtextbar.h"

namespace Digikam
{

class AlbumFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:

    AlbumFilterModel(QObject* parent = 0);

    /**
     * Sets the source model.
     * Note: If a chained filter model is set, it will not be reset, but
     * the source album model will be made source of the chained filter model.
     */
    void setSourceAlbumModel(AbstractAlbumModel* source);

    /**
     * Sets a chained filter model.
     * Note: If a direct source album model is set as current source, it will be set as
     * sourceAlbumModel of the new source filter model.
     */
    void setSourceFilterModel(AlbumFilterModel* source);

    AbstractAlbumModel* sourceAlbumModel() const;
    AlbumFilterModel*   sourceFilterModel() const;
    QModelIndex mapToSourceAlbumModel(const QModelIndex& index) const;
    QModelIndex mapFromSourceAlbumModel(const QModelIndex& index) const;

    /// Convenience methods
    Album* albumForIndex(const QModelIndex& index) const;
    QModelIndex indexForAlbum(Album* album) const;
    QModelIndex rootAlbumIndex() const;

    /**
     * Returns if the currently applied filters will result in any filtering.
     *
     * @return <code>true</code> if the current selected filter could result in
     *         any filtering without checking if this really happens.
     */
    virtual bool isFiltering() const;

    /**
     * Returns the settings currently used for filtering.
     *
     * @return current settings for filtering.
     */
    SearchTextSettings searchTextSettings() const;

    enum FilterBehavior
    {
        /** If an index does not matched, the index and all its children are filtered out.
         *  This is the Qt default behavior, but undesirable for album trees. */
        SimpleFiltering,
        /** Default behavior.
         *  If an index matches, it is shown, which directly means all its parents are shown as well.
         *  In addition, all its children are shown as well. */
        FullFiltering,
        /** If an index matches, it is shown, which directly means all its parents are shown as well.
         *  Its children are not shown unless they also match. */
        StrictFiltering
    };

    /**
     * Sets the filter behavior. Default is FullFiltering.
     */
    void setFilterBehavior(FilterBehavior behavior);

    enum MatchResult
    {
        /// This enum can be used as a boolean value if match/no match only is needed
        NoMatch = 0,
        /// The index itself is matched
        DirectMatch,
        /// A parent if the index is matched
        ParentMatch,
        /// A child of the index is matched
        ChildMatch,
        /// The index is matched not because of search settings, but because it has a special type
        SpecialMatch
    };
    /**
     * Returns the MatchResult of an index of this model.
     * Never returns NoMatch for a valid index, because in this case,
     * the index would rather be filtered out.
     **/
    MatchResult matchResult(const QModelIndex& index) const;

public Q_SLOTS:

    /**
     * Accepts new settings used for filtering and applies them to the model.
     *
     * @param settings new settings to apply. An empty text will be interpreted
     *                 as no filtering
     */
    void setSearchTextSettings(const SearchTextSettings& settings);

Q_SIGNALS:

    /**
     * This signal indicates that a new SearchTextSettings arrived and is about
     * to be applied to the model.
     *
     * @param searched <code>true</code> if filtering by text was enabled before
     *                 applying the new settings
     * @param willSearch <code>true</code> if the new settings can result in
     *                   any filtering by text, else <code> false.
     */
    void searchTextSettingsAboutToChange(bool searched, bool willSearch);

    /**
     * Indicates that new search text settings were applied.
     *
     * @param wasSearching <code>true</code> if this is not a new search that
     * @param searched <code>true</code> if the new settings result in any
     *                 filtering
     */
    void searchTextSettingsChanged(bool wasSearching, bool searched);

    /**
     * Indicates that a new filter was applied to the model.
     */
    void filterChanged();

    /**
     * Indicates whether the newly applied filter results in a search result or
     * not.
     *
     * @param hasResult <code>true</code> if the new filter matches any album,
     *                  else <code>false</code>
     */
    void hasSearchResult(bool hasResult);

protected:

    /**
     * Returns if the filter matches this album (same logic as filterAcceptsRow).
     * An album matches if the search text settings are found in a parent album's title,
     * in the album's title or in a child album's title, or if it is a special album (root)
     * that is never filtered out.
     **/
    MatchResult matchResult(Album* album) const;

    /**
     * This method provides the basic match checking algorithm.
     * Return true if this single album matches the current criteria.
     * This method can be overridden to provide custom filtering.
     *
     * @param album album to tell if it matches the filter criteria or not.
     */
    virtual bool matches(Album* album) const;

    /**
     * Use setSourceAlbumModel.
     *
     * @see setSourceAlbumModel
     * @param model source model
     */
    virtual void setSourceModel(QAbstractItemModel* model);

    virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;
    virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const;

protected Q_SLOTS:

    void slotAlbumRenamed(Album* album);
    void slotAlbumsHaveBeenUpdated(int type);

protected:

    FilterBehavior     m_filterBehavior;
    SearchTextSettings m_settings;
    QPointer<AlbumFilterModel> m_chainedModel;

private:

    /**
     * Tells whether the given settings result in filtering.
     *
     * @return <code>true</code> if the provided settings result in filtering
     *         the model
     */
    bool settingsFilter(const SearchTextSettings& settings) const;

};

/**
 * Filter model for checkable album models that allows more filtering options
 * based on check state.
 */
class CheckableAlbumFilterModel : public AlbumFilterModel
{
    Q_OBJECT
public:
    CheckableAlbumFilterModel(QObject* parent = 0);

    void setSourceAlbumModel(AbstractCheckableAlbumModel* source);
    void setSourceFilterModel(CheckableAlbumFilterModel* source);
    AbstractCheckableAlbumModel* sourceAlbumModel() const;

    void setFilterChecked(bool filter);
    void setFilterPartiallyChecked(bool filter);

    virtual bool isFiltering() const;

protected:

    virtual bool matches(Album* album) const;

    bool m_filterChecked;
    bool m_filterPartiallyChecked;
};

/**
 * Filter model for searches that can filter by search type
 */
class SearchFilterModel : public CheckableAlbumFilterModel
{
    Q_OBJECT

public:

    SearchFilterModel(QObject* parent = 0);
    void setSourceSearchModel(SearchModel* source);
    SearchModel* sourceSearchModel() const;

    /** Set the DatabaseSearch::Type. */
    void setFilterSearchType(DatabaseSearch::Type);
    void listAllSearches();
    void listNormalSearches();
    void listTimelineSearches();
    void listHaarSearches();
    void listMapSearches();
    void listDuplicatesSearches();

    /** Sets if temporary search albums shall be listed */
    void setListTemporarySearches(bool list);

    virtual bool isFiltering() const;

protected:

    virtual bool matches(Album* album) const;
    // make protected
    void setSourceAlbumModel(AbstractAlbumModel* source);

    void setTypeFilter(int type);

    int                     m_searchType;
    bool                    m_listTemporary;
};

/**
 * Filter model for tags that can filter by tag property
 */
class TagPropertiesFilterModel : public CheckableAlbumFilterModel
{
    Q_OBJECT

public:

    TagPropertiesFilterModel(QObject* parent = 0);
    void setSourceAlbumModel(TagModel* source);
    TagModel* sourceTagModel() const;

    void listOnlyTagsWithProperty(const QString& property);
    void removeListOnlyProperty(const QString& property);
    void doNotListTagsWithProperty(const QString& property);
    void removeDoNotListProperty(const QString& property);

    virtual bool isFiltering() const;

protected slots:

    void tagPropertiesChanged(TAlbum*);

protected:

    virtual bool matches(Album* album) const;

    QStringList        m_propertiesBlackList;
    QStringList        m_propertiesWhiteList;
};

} // namespace Digikam

#endif // ALBUMFILTERMODEL_H
