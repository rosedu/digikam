/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2008-02-26
 * Description : Upper widget in the search sidebar
 *
 * Copyright (C) 2008-2009 by Marcel Wiesweg <marcel dot wiesweg at gmx dot de>
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

#include "searchtabheader.moc"

// Qt includes

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedLayout>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

// KDE includes

#include <kdialog.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <klineedit.h>
#include <klocale.h>
#include <ksqueezedtextlabel.h>
#include <kurllabel.h>
#include <kdebug.h>

// Local includes

#include "album.h"
#include "albummanager.h"
#include "searchfolderview.h"
#include "searchwindow.h"
#include "searchxml.h"

namespace Digikam
{

class KeywordLineEdit : public KLineEdit
{
public:

    KeywordLineEdit(QWidget* parent = 0) : KLineEdit(parent)
    {
        m_hasAdvanced = false;
    }

    void showAdvancedSearch(bool hasAdvanced)
    {
        if (m_hasAdvanced == hasAdvanced)
        {
            return;
        }

        m_hasAdvanced = hasAdvanced;
        adjustStatus(m_hasAdvanced);
    }

    void focusInEvent(QFocusEvent* e)
    {
        if (m_hasAdvanced)
        {
            adjustStatus(false);
        }

        KLineEdit::focusInEvent(e);
    }

    void focusOutEvent(QFocusEvent* e)
    {
        KLineEdit::focusOutEvent(e);

        if (m_hasAdvanced)
        {
            adjustStatus(true);
        }
    }

    void adjustStatus(bool adv)
    {
        if (adv)
        {
            QPalette p = palette();
            p.setColor(QColorGroup::Text, p.color(QPalette::Disabled, QColorGroup::Text));
            setPalette(p);

            setText(i18n("(Advanced Search)"));
        }
        else
        {
            setPalette(QPalette());

            if (text() == i18n("(Advanced Search)"))
            {
                setText(QString());
            }
        }
    }

protected:

    bool m_hasAdvanced;
};

// -------------------------------------------------------------------------

class SearchTabHeaderPriv
{
public:

    SearchTabHeaderPriv() :
        newSearchWidget(0),
        saveAsWidget(0),
        editSimpleWidget(0),
        editAdvancedWidget(0),
        lowerArea(0),
        keywordEdit(0),
        advancedEditLabel(0),
        saveNameEdit(0),
        saveButton(0),
        storedKeywordEditName(0),
        storedKeywordEdit(0),
        storedAdvancedEditName(0),
        storedAdvancedEditLabel(0),
        keywordEditTimer(0),
        storedKeywordEditTimer(0),
        searchWindow(0),
        currentAlbum(0)
    {
    }

    QGroupBox*          newSearchWidget;
    QGroupBox*          saveAsWidget;
    QGroupBox*          editSimpleWidget;
    QGroupBox*          editAdvancedWidget;

    QStackedLayout*     lowerArea;

    KeywordLineEdit*    keywordEdit;
    QPushButton*        advancedEditLabel;

    KLineEdit*          saveNameEdit;
    QToolButton*        saveButton;

    KSqueezedTextLabel* storedKeywordEditName;
    KLineEdit*          storedKeywordEdit;
    KSqueezedTextLabel* storedAdvancedEditName;
    QPushButton*        storedAdvancedEditLabel;

    QTimer*             keywordEditTimer;
    QTimer*             storedKeywordEditTimer;

    SearchWindow*       searchWindow;

    SAlbum*             currentAlbum;

    QString             oldKeywordContent;
    QString             oldStoredKeywordContent;
};

SearchTabHeader::SearchTabHeader(QWidget* parent)
    : QWidget(parent), d(new SearchTabHeaderPriv)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);

    // upper part
    d->newSearchWidget = new QGroupBox(this);
    mainLayout->addWidget(d->newSearchWidget);

    // lower part
    d->lowerArea = new QStackedLayout;
    mainLayout->addLayout(d->lowerArea);

    d->saveAsWidget       = new QGroupBox(this);
    d->editSimpleWidget   = new QGroupBox(this);
    d->editAdvancedWidget = new QGroupBox(this);
    d->lowerArea->addWidget(d->saveAsWidget);
    d->lowerArea->addWidget(d->editSimpleWidget);
    d->lowerArea->addWidget(d->editAdvancedWidget);

    // ------------------- //

    // upper part

    d->newSearchWidget->setTitle(i18n("New Search"));
    QGridLayout* grid1   = new QGridLayout;
    QLabel* searchLabel  = new QLabel(i18n("Search:"), this);
    d->keywordEdit       = new KeywordLineEdit(this);
    d->keywordEdit->setClearButtonShown(true);
    d->keywordEdit->setClickMessage(i18n("Enter keywords here..."));

    d->advancedEditLabel = new QPushButton(i18n("Advanced Search..."), this);

    grid1->addWidget(searchLabel,          0, 0);
    grid1->addWidget(d->keywordEdit,       0, 1);
    grid1->addWidget(d->advancedEditLabel, 1, 1);
    grid1->setMargin(KDialog::spacingHint());
    grid1->setSpacing(KDialog::spacingHint());

    d->newSearchWidget->setLayout(grid1);

    // ------------------- //

    // lower part, variant 1

    d->saveAsWidget->setTitle(i18n("Save Current Search"));

    QHBoxLayout* hbox1 = new QHBoxLayout;
    d->saveNameEdit    = new KLineEdit(this);
    d->saveNameEdit->setWhatsThis(i18n("Enter a name for the current search to save it in the "
                                       "\"Searches\" view"));

    d->saveButton      = new QToolButton(this);
    d->saveButton->setIcon(SmallIcon("document-save"));
    d->saveButton->setToolTip(i18n("Save current search to a new virtual Album"));
    d->saveButton->setWhatsThis(i18n("If you press this button, the current search "
                                     "will be saved to a new virtual Search Album using the name "
                                     "set on the left side."));

    hbox1->addWidget(d->saveNameEdit);
    hbox1->addWidget(d->saveButton);
    hbox1->setMargin(KDialog::spacingHint());
    hbox1->setSpacing(KDialog::spacingHint());

    d->saveAsWidget->setLayout(hbox1);

    // ------------------- //

    // lower part, variant 2
    d->editSimpleWidget->setTitle(i18n("Edit Stored Search"));

    QVBoxLayout* vbox1       = new QVBoxLayout;
    d->storedKeywordEditName = new KSqueezedTextLabel(this);
    d->storedKeywordEditName->setTextElideMode(Qt::ElideRight);
    d->storedKeywordEdit     = new KLineEdit(this);

    vbox1->addWidget(d->storedKeywordEditName);
    vbox1->addWidget(d->storedKeywordEdit);
    vbox1->setMargin(KDialog::spacingHint());
    vbox1->setSpacing(KDialog::spacingHint());

    d->editSimpleWidget->setLayout(vbox1);

    // ------------------- //

    // lower part, variant 3
    d->editAdvancedWidget->setTitle(i18n("Edit Stored Search"));

    QVBoxLayout* vbox2 = new QVBoxLayout;

    d->storedAdvancedEditName = new KSqueezedTextLabel(this);
    d->storedAdvancedEditName->setTextElideMode(Qt::ElideRight);
    d->storedAdvancedEditLabel = new QPushButton(i18n("Edit..."), this);

    vbox2->addWidget(d->storedAdvancedEditName);
    vbox2->addWidget(d->storedAdvancedEditLabel);
    d->editAdvancedWidget->setLayout(vbox2);

    // ------------------- //

    // timers
    d->keywordEditTimer = new QTimer(this);
    d->keywordEditTimer->setSingleShot(true);
    d->keywordEditTimer->setInterval(800);

    d->storedKeywordEditTimer = new QTimer(this);
    d->storedKeywordEditTimer->setSingleShot(true);
    d->storedKeywordEditTimer->setInterval(800);

    // ------------------- //

    connect(d->keywordEdit, SIGNAL(textEdited(const QString&)),
            d->keywordEditTimer, SLOT(start()));

    connect(d->keywordEditTimer, SIGNAL(timeout()),
            this, SLOT(keywordChanged()));

    connect(d->keywordEdit, SIGNAL(editingFinished()),
            this, SLOT(keywordChanged()));

    connect(d->advancedEditLabel, SIGNAL(clicked()),
            this, SLOT(editCurrentAdvancedSearch()));

    connect(d->saveNameEdit, SIGNAL(returnPressed()),
            this, SLOT(saveSearch()));

    connect(d->saveButton, SIGNAL(clicked()),
            this, SLOT(saveSearch()));

    connect(d->storedKeywordEditTimer, SIGNAL(timeout()),
            this, SLOT(storedKeywordChanged()));

    connect(d->storedKeywordEdit, SIGNAL(editingFinished()),
            this, SLOT(storedKeywordChanged()));

    connect(d->storedAdvancedEditLabel, SIGNAL(clicked()),
            this, SLOT(editStoredAdvancedSearch()));
}

SearchTabHeader::~SearchTabHeader()
{
    delete d->searchWindow;
    delete d;
}

SearchWindow* SearchTabHeader::searchWindow()
{
    if (!d->searchWindow)
    {
        kDebug() << "Creating search window";
        // Create the advanced search edit window, deferred from constructor
        d->searchWindow = new SearchWindow;

        connect(d->searchWindow, SIGNAL(searchEdited(int, const QString&)),
                this, SLOT(advancedSearchEdited(int, const QString&)),
                Qt::QueuedConnection);
    }

    return d->searchWindow;
}

void SearchTabHeader::selectedSearchChanged(Album* a)
{

    SAlbum* album = dynamic_cast<SAlbum*> (a);

    // Signal from SearchFolderView that a search has been selected.

    // Don't check on d->currentAlbum == album, rather update status (which may have changed on same album)

    d->currentAlbum = album;

    kDebug() << "changing to SAlbum " << album;

    if (!album)
    {
        d->lowerArea->setCurrentWidget(d->saveAsWidget);
        d->lowerArea->setEnabled(false);
    }
    else
    {
        d->lowerArea->setEnabled(true);

        if (album->title() == SAlbum::getTemporaryTitle(DatabaseSearch::AdvancedSearch))
        {
            d->lowerArea->setCurrentWidget(d->saveAsWidget);

            if (album->isKeywordSearch())
            {
                d->keywordEdit->setText(keywordsFromQuery(album->query()));
                d->keywordEdit->showAdvancedSearch(false);
            }
            else
            {
                d->keywordEdit->showAdvancedSearch(true);
            }
        }
        else if (album->isKeywordSearch())
        {
            d->lowerArea->setCurrentWidget(d->editSimpleWidget);
            d->storedKeywordEditName->setText(album->title());
            d->storedKeywordEdit->setText(keywordsFromQuery(album->query()));
            d->keywordEdit->showAdvancedSearch(false);
        }
        else
        {
            d->lowerArea->setCurrentWidget(d->editAdvancedWidget);
            d->storedAdvancedEditName->setText(album->title());
            d->keywordEdit->showAdvancedSearch(false);
        }
    }
}

void SearchTabHeader::editSearch(SAlbum* album)
{
    if (!album)
    {
        return;
    }

    if (album->isAdvancedSearch())
    {
        SearchWindow* window = searchWindow();
        window->readSearch(album->id(), album->query());
        window->show();
        window->raise();
    }
    else if (album->isKeywordSearch())
    {
        d->storedKeywordEdit->selectAll();
    }
}

void SearchTabHeader::newKeywordSearch()
{
    d->keywordEdit->clear();
    QString keywords = d->keywordEdit->text();
    setCurrentSearch(DatabaseSearch::KeywordSearch, queryFromKeywords(keywords));
    d->keywordEdit->setFocus();
}

void SearchTabHeader::newAdvancedSearch()
{
    SearchWindow* window = searchWindow();
    window->reset();
    window->show();
    window->raise();
}

void SearchTabHeader::keywordChanged()
{
    QString keywords = d->keywordEdit->text();
    kDebug() << "keywords changed to '" << keywords << "'";

    if (d->oldKeywordContent == keywords || keywords.trimmed().isEmpty())
    {
        kDebug() << "same keywords as before, ignoring...";
        return;
    }
    else
    {
        d->oldKeywordContent = keywords;
    }

    setCurrentSearch(DatabaseSearch::KeywordSearch, queryFromKeywords(keywords));
    d->keywordEdit->setFocus();
}

void SearchTabHeader::editCurrentAdvancedSearch()
{
    SAlbum* album = AlbumManager::instance()->findSAlbum(SAlbum::getTemporaryTitle(DatabaseSearch::AdvancedSearch));
    SearchWindow* window = searchWindow();

    if (album)
    {
        window->readSearch(album->id(), album->query());
    }
    else
    {
        window->reset();
    }

    window->show();
    window->raise();
}

void SearchTabHeader::saveSearch()
{
    // Only applicable if:
    // 1. current album is Search View Current Album Save this album as a user names search album.
    // 2. user as processed a search before to save it.

    QString name = d->saveNameEdit->text();

    kDebug() << "name = " << name;

    if (name.isEmpty() || !d->currentAlbum)
    {
        kDebug() << "no current album, returning";
        // passive popup
        return;
    }

    SAlbum* oldAlbum = AlbumManager::instance()->findSAlbum(name);

    while (oldAlbum)
    {
        QString label = i18n( "Search name already exists."
                              "\nPlease enter a new name:" );
        bool ok;
        QString newTitle = KInputDialog::getText( i18n("Name exists"), label,
                           name, &ok, this );

        if (!ok)
        {
            return;
        }

        name     = newTitle;
        oldAlbum = AlbumManager::instance()->findSAlbum(name);
    }

    SAlbum* newAlbum = AlbumManager::instance()->createSAlbum(name, d->currentAlbum->searchType(),
                       d->currentAlbum->query());
    emit searchShallBeSelected(newAlbum);
}

void SearchTabHeader::storedKeywordChanged()
{
    QString keywords = d->storedKeywordEdit->text();

    if (d->oldStoredKeywordContent == keywords)
    {
        return;
    }
    else
    {
        d->oldStoredKeywordContent = keywords;
    }

    if (d->currentAlbum)
    {
        AlbumManager::instance()->updateSAlbum(d->currentAlbum, queryFromKeywords(keywords));
        emit searchShallBeSelected(d->currentAlbum);
    }
}

void SearchTabHeader::editStoredAdvancedSearch()
{
    if (d->currentAlbum)
    {
        SearchWindow* window = searchWindow();
        window->readSearch(d->currentAlbum->id(), d->currentAlbum->query());
        window->show();
        window->raise();
    }
}

void SearchTabHeader::advancedSearchEdited(int id, const QString& query)
{
    // if the user just pressed the button, but did not change anything in the window,
    // the search is effectively still a keyword search.
    // We go the hard way and check this case.
    KeywordSearchReader check(query);
    DatabaseSearch::Type type = check.isSimpleKeywordSearch() ? DatabaseSearch::KeywordSearch
                                : DatabaseSearch::AdvancedSearch;

    if (id == -1)
    {
        setCurrentSearch(type, query);
    }
    else
    {
        SAlbum* album = AlbumManager::instance()->findSAlbum(id);

        if (album)
        {
            AlbumManager::instance()->updateSAlbum(album, query, album->title(), type);
            emit searchShallBeSelected(album);
        }
    }
}

void SearchTabHeader::setCurrentSearch(DatabaseSearch::Type type, const QString& query, bool selectCurrentAlbum)
{
    SAlbum* album = AlbumManager::instance()->findSAlbum(SAlbum::getTemporaryTitle(DatabaseSearch::KeywordSearch));

    if (album)
    {
        AlbumManager::instance()->updateSAlbum(album, query,
                                               SAlbum::getTemporaryTitle(DatabaseSearch::KeywordSearch),
                                               type);
    }
    else
    {
        album = AlbumManager::instance()->createSAlbum(SAlbum::getTemporaryTitle(DatabaseSearch::KeywordSearch),
                type, query);
    }

    if (selectCurrentAlbum)
    {
        emit searchShallBeSelected(album);
    }
}

QString SearchTabHeader::queryFromKeywords(const QString& keywords)
{
    QStringList keywordList = KeywordSearch::split(keywords);
    // create xml
    KeywordSearchWriter writer;
    return writer.xml(keywordList);
}

QString SearchTabHeader::keywordsFromQuery(const QString& query)
{
    KeywordSearchReader reader(query);
    QStringList keywordList = reader.keywords();
    return KeywordSearch::merge(keywordList);
}

} // namespace Digikam
