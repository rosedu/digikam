/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2010-12-02
 * Description : Generic, standard item based model for KCategorizedView
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

#ifndef CATEGORIZEDITEMMODEL_H
#define CATEGORIZEDITEMMODEL_H

// Qt includes

#include <QStandardItemModel>

// KDE includes

#include <KCategorizedSortFilterProxyModel>

// Local includes

#include "digikam_export.h"

class QAction;
class KCategorizedSortFilterProxyModel;

namespace Digikam
{

class DIGIKAM_EXPORT CategorizedItemModel : public QStandardItemModel
{
    Q_OBJECT

public:

    enum ExtraRoles
    {
        /// This role, per default, reflects the order in which items are added
        ItemOrderRole = Qt::UserRole + 1
    };

    CategorizedItemModel(QObject* parent = 0);

    QStandardItem *addItem(const QString& text, const QVariant& category, const QVariant& categorySorting = QVariant());
    QStandardItem *addItem(const QString& text, const QIcon& decoration, const QVariant& category,
                           const QVariant& categorySorting = QVariant());

    KCategorizedSortFilterProxyModel *createFilterModel();
};

class DIGIKAM_EXPORT ActionItemModel : public CategorizedItemModel
{
    Q_OBJECT

public:

    enum ExtraRoles
    {
        ItemActionRole = Qt::UserRole + 10
    };

    /**
     * This class is a CategorizedItemModel based on QActions, taking an action's text and icon
     * for display and decoration.
     * It is possible to retrieve an action for an index, and to call the action's slots from
     * a given index.
     */

    ActionItemModel(QObject* parent = 0);

    QStandardItem *addAction(QAction *action, const QString& category, const QVariant& categorySorting = QVariant());

    enum MenuCategoryFlag
    {
        /// The toplevel menu's text is used as category
        ToplevelMenuCategory = 1 << 0,
        /// If the action is in a submenu, this menu's text is taken as category
        ParentMenuCategory   = 1 << 1,

        /// Sort categories alphabetically by category name
        SortCategoriesAlphabetically   = 1 << 10,
        /// Sort categories by the order they are added (found in the scanned menu)
        SortCategoriesByInsertionOrder = 1 << 11
    };
    Q_DECLARE_FLAGS(MenuCategoryMode, MenuCategoryFlag)

    void setMode(MenuCategoryMode mode);
    MenuCategoryMode mode() const;

    void addActions(QWidget* widget);
    void addActions(QWidget* widget, const QList<QAction*>& actionWhiteList);

    /**
     * Returns the action for the given index.
     * The method can also be used for indices from proxy models.
     */
    static QAction *actionForIndex(const QModelIndex& index);
    /**
     * Returns the action for the given index.
     * Note: these methods perform O(n).
     */
    QStandardItem* itemForAction(QAction *action) const;
    QModelIndex indexForAction(QAction *action) const;

public Q_SLOTS:

    /**
     * These three slots will cause the slots of the referred action to be called.
     * Connect here for example a view's signals.
     * Note that you can also pass indices from proxy models.
     */
    void hover(const QModelIndex& index);
    void toggle(const QModelIndex& index);
    void trigger(const QModelIndex& index);

protected Q_SLOTS:

    void slotActionChanged();

protected:

    void setPropertiesFromAction(QStandardItem *item, QAction* action);

    MenuCategoryMode m_mode;
};

} // namespace

Q_DECLARE_OPERATORS_FOR_FLAGS(Digikam::ActionItemModel::MenuCategoryMode)

#endif // CATEGORIZEDITEMMODEL_H

