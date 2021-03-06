/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2009-11-11
 * Description : a modifier for setting a default value if option parsing failed
 *
 * Copyright (C) 2009 by Andi Clemens <andi dot clemens at gmx dot net>
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

#include "defaultvaluemodifier.moc"

// Qt includes

#include <QGridLayout>
#include <QLabel>
#include <QPointer>

// KDE includes

#include <kiconloader.h>
#include <klineedit.h>
#include <klocale.h>

namespace Digikam
{

DefaultValueDialog::DefaultValueDialog(Parseable* parent)
    : ParseableDialog(parent),
      valueInput(0)
{
    QString defaultValueStr = i18n("Default Value");

    QLabel* srcLabel = new QLabel(defaultValueStr + ':');
    valueInput       = new KLineEdit(this);
    valueInput->setToolTip(i18n("<p>Set a default value for empty strings.<br/>"
                                "When applied to a renaming option, "
                                "an empty string will be replaced by the value you specify here.</p>"));

    QWidget*     mainWidget = new QWidget(this);
    QGridLayout* mainLayout = new QGridLayout(this);
    mainLayout->addWidget(srcLabel,   0, 0);
    mainLayout->addWidget(valueInput, 0, 1);
    mainLayout->setSpacing(KDialog::spacingHint());
    mainLayout->setMargin(KDialog::spacingHint());
    mainLayout->setRowStretch(1, 10);
    mainWidget->setLayout(mainLayout);

    setSettingsWidget(mainWidget);

    valueInput->setFocus();
}

DefaultValueDialog::~DefaultValueDialog()
{
}

// --------------------------------------------------------

DefaultValueModifier::DefaultValueModifier()
    : Modifier(i18nc("default value for empty strings", "Default Value..."),
               i18n("Set a default value for empty strings"),
               SmallIcon("edit-undo"))
{
    addToken("{default:\"||value||\"}", description());

    QRegExp reg("\\{default:\"(.+)\"\\}");
    reg.setMinimal(true);
    setRegExp(reg);
}

void DefaultValueModifier::slotTokenTriggered(const QString& token)
{
    Q_UNUSED(token)

    QString result;

    QPointer<DefaultValueDialog> dlg = new DefaultValueDialog(this);

    if (dlg->exec() == KDialog::Accepted)
    {
        QString valueStr = dlg->valueInput->text();

        if (!valueStr.isEmpty())
        {
            result = QString("{default:\"%1\"}").arg(valueStr);
        }
    }

    delete dlg;

    emit signalTokenTriggered(result);
}

QString DefaultValueModifier::parseOperation(ParseSettings& settings)
{
    Q_UNUSED(settings);

    if (!settings.str2Modify.isEmpty())
    {
        return settings.str2Modify;
    }

    const QRegExp& reg = regExp();

    QString defaultStr = reg.cap(1).isEmpty() ? QString() : reg.cap(1);

    return defaultStr;
}

} // namespace Digikam
