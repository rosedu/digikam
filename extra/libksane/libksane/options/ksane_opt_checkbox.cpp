/* ============================================================
 *
 * This file is part of the KDE project
 *
 * Date        : 2009-01-21
 * Description : Sane interface for KDE
 *
 * Copyright (C) 2009 by Kare Sars <kare dot sars at iki dot fi>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ============================================================ */
// Local includes.
#include "ksane_opt_checkbox.h"
#include "ksane_opt_checkbox.moc"

#include "labeled_checkbox.h"

// Qt includes
#include <QtCore/QVarLengthArray>

// KDE includes
#include <KDebug>
#include <KLocale>

namespace KSaneIface
{

KSaneOptCheckBox::KSaneOptCheckBox(const SANE_Handle handle, const int index)
: KSaneOption(handle, index), m_checkbox(0)
{
}

void KSaneOptCheckBox::createWidget(QWidget *parent)
{
    if (m_frame) return;

    readOption();

    if (!m_optDesc) return;

    m_frame = m_checkbox = new LabeledCheckbox(parent, i18n(m_optDesc->title));
    m_frame->setToolTip(i18n(m_optDesc->desc));

    connect(m_checkbox, SIGNAL(toggled(bool)), this, SLOT(checkboxChanged(bool)));

    updateVisibility();
    readValue();
}


void KSaneOptCheckBox::widgetSizeHints(int *lab_w, int *rest_w)
{
    if (m_checkbox) {
        m_checkbox->widgetSizeHints(lab_w, rest_w);
    }
}
void KSaneOptCheckBox::setColumnWidths(int lab_w, int rest_w)
{
    if (m_checkbox) {
        m_checkbox->setColumnWidths(lab_w, rest_w);
    }
}

void KSaneOptCheckBox::checkboxChanged(bool toggled)
{
    unsigned char data[4];
    
    fromSANE_Word(data, (toggled) ? 1:0);
    writeData(data);
}

void KSaneOptCheckBox::readValue()
{
    if (state() == STATE_HIDDEN) return;

    // read that current value
    QVarLengthArray<unsigned char> data(m_optDesc->size);
    SANE_Status status;
    SANE_Int res;
    status = sane_control_option (m_handle, m_index, SANE_ACTION_GET_VALUE, data.data(), &res);
    if (status != SANE_STATUS_GOOD) {
        return;
    }
    
    m_checked = (toSANE_Word(data.data()) != 0) ? true:false;
    if (m_checkbox) {
        m_checkbox->setChecked(m_checked);
    }
}

bool KSaneOptCheckBox::getValue(float &val)
{
    if (state() == STATE_HIDDEN) return false;
    val = m_checked ? 1.0 : 0.0;
    return true;
}

bool KSaneOptCheckBox::setValue(float val)
{
    if (state() == STATE_HIDDEN) return false;
    checkboxChanged(val == 0);
    readValue();
    return true;
}

bool KSaneOptCheckBox::getValue(QString &val)
{
    if (state() == STATE_HIDDEN) return false;
    val = m_checked ? "true" : "false";
    return true;
}

bool KSaneOptCheckBox::setValue(const QString &val)
{
    if (state() == STATE_HIDDEN) return false;
    if ((val.compare("true", Qt::CaseInsensitive) == 0) ||
        (val.compare("1") == 0))
    {
        checkboxChanged(true);
    }
    else {
        checkboxChanged(false);
    }
    return true;
}


}  // NameSpace KSaneIface
