/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2004-11-28
 * Description : a digiKam image editor plugin to process image
 *               free rotation.
 *
 * Copyright (C) 2004-2010 by Gilles Caulier <caulier dot gilles at gmail dot com>
 * Copyright (C) 2009-2010 by Andi Clemens <andi dot clemens at gmx dot net>
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

#include "freerotationtool.moc"

// C++ includes

#include <cmath>

// Qt includes

#include <QGridLayout>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPushButton>

// KDE includes

#include <kapplication.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kdialog.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kseparator.h>

// LibKDcraw includes

#include <libkdcraw/rexpanderbox.h>

// Local includes

#include "dimg.h"
#include "editortoolsettings.h"
#include "freerotationfilter.h"
#include "freerotationsettings.h"
#include "imageiface.h"
#include "imageguidewidget.h"

using namespace KDcrawIface;

namespace DigikamTransformImagePlugin
{

class FreeRotationTool::FreeRotationToolPriv
{
public:

    FreeRotationToolPriv() :
        configGroupName("freerotation Tool"),

        newHeightLabel(0),
        newWidthLabel(0),
        autoAdjustBtn(0),
        autoAdjustPoint1Btn(0),
        autoAdjustPoint2Btn(0),
        expanderBox(0),
        gboxSettings(0),
        previewWidget(0)
    {}

    const QString         configGroupName;

    QLabel*               newHeightLabel;
    QLabel*               newWidthLabel;

    QPoint                autoAdjustPoint1;
    QPoint                autoAdjustPoint2;

    QPushButton*          autoAdjustBtn;
    QPushButton*          autoAdjustPoint1Btn;
    QPushButton*          autoAdjustPoint2Btn;

    FreeRotationSettings* settingsView;

    RExpanderBox*         expanderBox;
    EditorToolSettings*   gboxSettings;
    ImageGuideWidget*     previewWidget;
};

FreeRotationTool::FreeRotationTool(QObject* parent)
    : EditorToolThreaded(parent),
      d(new FreeRotationToolPriv)
{
    setObjectName("freerotation");
    setToolName(i18n("Free Rotation"));
    setToolIcon(SmallIcon("freerotation"));

    d->previewWidget = new ImageGuideWidget(0, true, ImageGuideWidget::HVGuideMode);
    d->previewWidget->setWhatsThis(i18n("This is the free rotation operation preview. "
                                        "If you move the mouse cursor on this preview, "
                                        "a vertical and horizontal dashed line will be drawn "
                                        "to guide you in adjusting the free rotation correction. "
                                        "Release the left mouse button to freeze the dashed "
                                        "line's position."));

    setToolView(d->previewWidget);
    setPreviewModeMask(PreviewToolBar::UnSplitPreviewModes);

    // -------------------------------------------------------------

    QString temp;
    ImageIface iface(0, 0);

    d->gboxSettings   = new EditorToolSettings;
    d->gboxSettings->setTools(EditorToolSettings::ColorGuide);

    QLabel* label1    = new QLabel(i18n("New width:"));
    d->newWidthLabel  = new QLabel(temp.setNum( iface.originalWidth()) + i18n(" px"));
    d->newWidthLabel->setAlignment( Qt::AlignBottom | Qt::AlignRight );

    QLabel* label2    = new QLabel(i18n("New height:"));
    d->newHeightLabel = new QLabel(temp.setNum( iface.originalHeight()) + i18n(" px"));
    d->newHeightLabel->setAlignment( Qt::AlignBottom | Qt::AlignRight );

    // -------------------------------------------------------------

    QString btnWhatsThis = i18n("Select a point in the preview widget, "
                                "then click this button to assign the point for auto-correction.");

    QPixmap pm1 = generateBtnPixmap(QString("1"), Qt::black);
    d->autoAdjustPoint1Btn = new QPushButton;
    d->autoAdjustPoint1Btn->setIcon(pm1);
    d->autoAdjustPoint1Btn->setSizePolicy(QSizePolicy::MinimumExpanding,
                                          QSizePolicy::MinimumExpanding);

    QPixmap pm2 = generateBtnPixmap(QString("2"), Qt::black);
    d->autoAdjustPoint2Btn = new QPushButton;
    d->autoAdjustPoint2Btn->setIcon(pm2);
    d->autoAdjustPoint2Btn->setSizePolicy(QSizePolicy::MinimumExpanding,
                                          QSizePolicy::MinimumExpanding);

    d->autoAdjustPoint1Btn->setShortcut(QKeySequence(Qt::Key_1));
    d->autoAdjustPoint1Btn->setToolTip(btnWhatsThis);
    d->autoAdjustPoint1Btn->setWhatsThis(btnWhatsThis);
    d->autoAdjustPoint2Btn->setToolTip(btnWhatsThis);
    d->autoAdjustPoint2Btn->setWhatsThis(btnWhatsThis);

    // --------------------------------------------------------

    // try to determine the maximum text width, to set the button minwidth
    QPoint p;
    setPointInvalid(p);
    QString invalidText = generateButtonLabel(p);
    p.setX(1);
    p.setY(2);
    QString validText   = generateButtonLabel(p);

    QFont fnt = d->autoAdjustPoint1Btn->font();
    QFontMetrics fm(fnt);

    const int offset = (pm1.width() * 2) + 10;
    int minWidth1    = fm.width(invalidText) + offset;
    int minWidth2    = fm.width(validText) + offset;
    int minWidth     = qMax<int>(minWidth1, minWidth2);

    // set new minwidth
    d->autoAdjustPoint1Btn->setMinimumWidth(minWidth);
    d->autoAdjustPoint2Btn->setMinimumWidth(minWidth);

    // --------------------------------------------------------

    d->autoAdjustPoint1Btn->setText(invalidText);
    d->autoAdjustPoint2Btn->setText(invalidText);

    d->autoAdjustBtn = new QPushButton(i18nc("Automatic Adjustment", "Adjust"));
    d->autoAdjustBtn->setSizePolicy(QSizePolicy::MinimumExpanding,
                                    QSizePolicy::Expanding);

    // --------------------------------------------------------

    QWidget* autoAdjustContainer  = new QWidget;
    QGridLayout* containerLayout2 = new QGridLayout;
    QLabel* autoDescr             = new QLabel;
    autoDescr->setText(i18n("<p>Correct the rotation of your images automatically by assigning two points in the "
                            "preview widget and clicking <i>Adjust</i>.<br/>"
                            "You can either adjust horizontal or vertical lines.</p>"));
    autoDescr->setAlignment(Qt::AlignJustify);
    autoDescr->setWordWrap(true);
    containerLayout2->addWidget(autoDescr,              0, 0, 1,-1);
    containerLayout2->addWidget(d->autoAdjustPoint1Btn, 1, 0, 1, 1);
    containerLayout2->addWidget(d->autoAdjustBtn,       1, 2, 2, 1);
    containerLayout2->addWidget(d->autoAdjustPoint2Btn, 2, 0, 1, 1);
    containerLayout2->setColumnStretch(1, 10);
    containerLayout2->setMargin(KDialog::marginHint());
    autoAdjustContainer->setLayout(containerLayout2);

    // -------------------------------------------------------------

    KSeparator* line  = new KSeparator(Qt::Horizontal);
    d->settingsView   = new FreeRotationSettings(d->gboxSettings->plainPage());

    d->expanderBox    = new RExpanderBox;
    d->expanderBox->setObjectName("FreeRotationTool Expander");
    d->expanderBox->addItem(autoAdjustContainer, SmallIcon("freerotation"), i18n("Automatic Adjustment"),
                            QString("AutoAdjustContainer"), true);
    d->expanderBox->addItem(d->settingsView, SmallIcon("freerotation"), i18n("Settings"),
                            QString("SettingsContainer"), true);
    d->expanderBox->addStretch();

    // -------------------------------------------------------------

    QGridLayout* grid2 = new QGridLayout;
    grid2->addWidget(label1,            0, 0, 1, 1);
    grid2->addWidget(d->newWidthLabel,  0, 1, 1, 1);
    grid2->addWidget(label2,            1, 0, 1, 1);
    grid2->addWidget(d->newHeightLabel, 1, 1, 1, 1);
    grid2->addWidget(line,              2, 0, 1,-1);
    grid2->addWidget(d->expanderBox,    3, 0, 1,-1);
    grid2->setRowStretch(3, 10);
    grid2->setMargin(d->gboxSettings->spacingHint());
    grid2->setSpacing(d->gboxSettings->spacingHint());
    d->gboxSettings->plainPage()->setLayout(grid2);

    setToolSettings(d->gboxSettings);
    init();

    // -------------------------------------------------------------

    connect(d->settingsView, SIGNAL(signalSettingsChanged()),
            this, SLOT(slotTimer()));

    connect(d->gboxSettings, SIGNAL(signalColorGuideChanged()),
            this, SLOT(slotColorGuideChanged()));

    connect(d->autoAdjustPoint1Btn, SIGNAL(clicked()),
            this, SLOT(slotAutoAdjustP1Clicked()));

    connect(d->autoAdjustPoint2Btn, SIGNAL(clicked()),
            this, SLOT(slotAutoAdjustP2Clicked()));

    connect(d->autoAdjustBtn, SIGNAL(clicked()),
            this, SLOT(slotAutoAdjustClicked()));
}

FreeRotationTool::~FreeRotationTool()
{
    delete d;
}

void FreeRotationTool::slotColorGuideChanged()
{
    d->previewWidget->slotChangeGuideColor(d->gboxSettings->guideColor());
    d->previewWidget->slotChangeGuideSize(d->gboxSettings->guideSize());
}

void FreeRotationTool::readSettings()
{
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup group        = config->group(d->configGroupName);

    d->settingsView->readSettings(group);
    d->expanderBox->readSettings();

    resetPoints();
    slotColorGuideChanged();
}

void FreeRotationTool::writeSettings()
{
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup group        = config->group(d->configGroupName);
    d->settingsView->writeSettings(group);

    group.sync();
}

void FreeRotationTool::slotResetSettings()
{
    d->settingsView->resetToDefault();
    resetPoints();
    slotEffect();
}

void FreeRotationTool::prepareEffect()
{
    FreeRotationContainer settings = d->settingsView->settings();
    ImageIface* iface              = d->previewWidget->imageIface();
    DImg preview                   = iface->getPreviewImg();
    settings.backgroundColor       = toolView()->backgroundRole();
    settings.orgW                  = iface->originalWidth();
    settings.orgH                  = iface->originalHeight();
    setFilter(new FreeRotationFilter(&preview, this, settings));
}

void FreeRotationTool::prepareFinal()
{
    ImageIface iface(0, 0);
    FreeRotationContainer settings = d->settingsView->settings();
    DImg* orgImage                 = iface.getOriginalImg();
    settings.backgroundColor       = Qt::black;
    settings.orgW                  = iface.originalWidth();
    settings.orgH                  = iface.originalHeight();

    setFilter(new FreeRotationFilter(orgImage, this, settings));
}

void FreeRotationTool::putPreviewData()
{
    ImageIface* iface = d->previewWidget->imageIface();
    int w             = iface->previewWidth();
    int h             = iface->previewHeight();

    DImg imTemp = filter()->getTargetImage().smoothScale(w, h, Qt::KeepAspectRatio);
    DImg imDest(w, h, filter()->getTargetImage().sixteenBit(), filter()->getTargetImage().hasAlpha());

    QColor background = toolView()->backgroundRole();
    imDest.fill(DColor(background, filter()->getTargetImage().sixteenBit()));
    imDest.bitBltImage(&imTemp, (w-imTemp.width())/2, (h-imTemp.height())/2);

    iface->putPreviewImage((imDest.smoothScale(iface->previewWidth(), iface->previewHeight())).bits());
    d->previewWidget->updatePreview();

    QString temp;
    QSize newSize = dynamic_cast<FreeRotationFilter*>(filter())->getNewSize();
    int new_w     = (newSize.width()  == -1) ? iface->originalWidth()  : newSize.width();
    int new_h     = (newSize.height() == -1) ? iface->originalHeight() : newSize.height();
    d->newWidthLabel->setText(temp.setNum(new_w)  + i18n(" px") );
    d->newHeightLabel->setText(temp.setNum(new_h) + i18n(" px") );
}

void FreeRotationTool::putFinalData()
{
    ImageIface iface(0, 0);
    DImg targetImage = filter()->getTargetImage();
    iface.putOriginalImage(i18n("Free Rotation"), filter()->filterAction(), targetImage.bits(), targetImage.width(), targetImage.height());
}

QString FreeRotationTool::generateButtonLabel(const QPoint& p)
{
    QString clickToSet     = i18n("Click to set");
    QString isOk           = i18nc("point has been set and is valid", "Okay");
    bool clickToSetIsWider = clickToSet.count() >= isOk.count();
    QString widestString   = clickToSetIsWider ? clickToSet : isOk;
    int maxLength          = widestString.count();
    QString label          = clickToSetIsWider ? clickToSet : centerString(clickToSet, maxLength);

    if (pointIsValid(p))
    {
        label = clickToSetIsWider ? centerString(isOk, maxLength) : isOk;
    }

    return label;
}

QString FreeRotationTool::centerString(const QString& str, int maxLength)
{
    QString tmp = str;
    int max     = (maxLength == -1) ? tmp.count() : maxLength;

    // fill with additional whitespace, to match the original label length and center
    // the text, without moving the button icon
    int diff = qAbs<int>(max - str.count());

    if (diff > 0)
    {
        QString delimiter(" ");
        int times = (diff / 2);

#if QT_VERSION >= 0x040500
        tmp.prepend(delimiter.repeated(times));
        tmp.append(delimiter.repeated(times));
#else
        tmp.prepend(repeatString(delimiter, times));
        tmp.append(repeatString(delimiter, times));
#endif

        diff = qAbs<int>(maxLength - tmp.count());

        if (diff != 0)
        {
            // too long?
            if (tmp.count() > maxLength)
            {
                tmp.chop(diff);
            }
            // too short?
            else if (tmp.count() < maxLength)
            {
#if QT_VERSION >= 0x040500
                tmp.append(delimiter.repeated(diff));
#else
                tmp.append(repeatString(delimiter, diff));
#endif
            }
        }
    }

    return tmp;
}

void FreeRotationTool::updatePoints()
{
    // set labels
    QString tmp = generateButtonLabel(d->autoAdjustPoint1);
    d->autoAdjustPoint1Btn->setText(tmp);

    tmp = generateButtonLabel(d->autoAdjustPoint2);
    d->autoAdjustPoint2Btn->setText(tmp);

    // set points in preview widget, don't add invalid points
    QPolygon points;

    if (pointIsValid(d->autoAdjustPoint1))
    {
        points << d->autoAdjustPoint1;
        d->autoAdjustPoint2Btn->setEnabled(true);
    }
    else
    {
        d->autoAdjustPoint2Btn->setEnabled(false);
    }

    if (pointIsValid(d->autoAdjustPoint2))
    {
        points << d->autoAdjustPoint2;
    }

    d->previewWidget->setPoints(points, true);

    // enable / disable adjustment buttons
    bool valid  = (pointIsValid(d->autoAdjustPoint1)  &&
                   pointIsValid(d->autoAdjustPoint2)) &&
                  (d->autoAdjustPoint1 != d->autoAdjustPoint2);
    d->autoAdjustBtn->setEnabled(valid);
}

void FreeRotationTool::resetPoints()
{
    setPointInvalid(d->autoAdjustPoint1);
    setPointInvalid(d->autoAdjustPoint2);
    d->previewWidget->resetPoints();
    updatePoints();
}

void FreeRotationTool::slotAutoAdjustP1Clicked()
{
    d->autoAdjustPoint1 = d->previewWidget->getSpotPosition();
    updatePoints();
}

void FreeRotationTool::slotAutoAdjustP2Clicked()
{
    d->autoAdjustPoint2 = d->previewWidget->getSpotPosition();
    updatePoints();
}

void FreeRotationTool::slotAutoAdjustClicked()
{
    // we need to check manually here if the button is enabled, because this slot can be called
    // with an action now
    if (!d->autoAdjustBtn->isEnabled())
    {
        return;
    }

    double angle = calculateAutoAngle();

    if (fabs(angle) > 45.0)
    {
        if (angle < 0.0)
        {
            angle += 90.0;
        }
        else
        {
            angle -= 90.0;
        }
    }

    // we need to add the calculated angle to the currently set angle
    angle                  = d->settingsView->settings().angle + angle;

    // convert the angle to a string so we can easily split it up
    QString angleStr       = QString::number(angle, 'f', 2);
    QStringList anglesList = angleStr.split('.');

    // try to set the angle widgets with the extracted values
    if (anglesList.count() == 2)
    {
        bool ok       = false;
        int mainAngle = anglesList[0].toInt(&ok);

        if (!ok)
        {
            mainAngle = 0;
        }

        double fineAngle = (QString("0.") + anglesList[1]).toDouble(&ok);
        fineAngle        = (angle < 0.0) ? -fineAngle : fineAngle;

        if (!ok)
        {
            fineAngle = 0.0;
        }

        FreeRotationContainer prm = d->settingsView->settings();
        prm.angle                 = mainAngle + fineAngle;
        d->settingsView->setSettings(prm);
        slotEffect();
    }

    resetPoints();
}

QPixmap FreeRotationTool::generateBtnPixmap(const QString& label, const QColor& color)
{
    QPixmap pm(22, 22);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(color);

    p.drawEllipse(1, 1, 20, 20);
    p.drawText(pm.rect(), label, Qt::AlignHCenter | Qt::AlignVCenter);

    p.end();

    return pm;
}

double FreeRotationTool::calculateAutoAngle()
{
    // check if all points are valid
    if (!pointIsValid(d->autoAdjustPoint1) && !pointIsValid(d->autoAdjustPoint2))
    {
        return 0.0;
    }

    return FreeRotationFilter::calculateAngle(d->autoAdjustPoint1, d->autoAdjustPoint2);
}

void FreeRotationTool::setPointInvalid(QPoint& p)
{
    p.setX(-1);
    p.setY(-1);
}

bool FreeRotationTool::pointIsValid(const QPoint& p)
{
    bool valid = true;

    if (p.x() == -1 || p.y() == -1)
    {
        valid = false;
    }

    return valid;
}

QString FreeRotationTool::repeatString(const QString& str, int times)
{
    QString tmp;

    for (int i = 0; i < times; ++i)
    {
        tmp.append(str);
    }

    return tmp;
}

}  // namespace DigikamTransformImagePlugin