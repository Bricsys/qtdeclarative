// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#undef QT_NO_FOREACH // this file contains unported legacy Q_FOREACH uses

#include "qquickparticleaffector_p.h"
#include <QDebug>
#include <private/qqmlglobal_p.h>
QT_BEGIN_NAMESPACE

/*!
    \qmltype ParticleAffector
//!    \nativetype QQuickParticleAffector
    \inqmlmodule QtQuick.Particles
    \brief Applies alterations to the attributes of logical particles at any
    point in their lifetime.
    \ingroup qtquick-particles

    The base ParticleAffector does not alter any attributes, but can be used to emit a signal
    when a particle meets certain conditions.

    If an affector has a defined size, then it will only affect particles within its size and
position on screen.

    Affectors have different performance characteristics to the other particle system elements. In
particular, they have some simplifications to try to maintain a simulation at real-time or faster.
When running a system with Affectors, irregular frame timings that grow too large ( > one second per
frame) will cause the Affectors to try and cut corners with a faster but less accurate simulation.
If the system has multiple affectors the order in which they are applied is not guaranteed, and when
simulating larger time shifts they will simulate the whole shift each, which can lead to different
results compared to smaller time shifts.

    Accurate simulation for large numbers of particles (hundreds) with multiple affectors may be
possible on some hardware, but on less capable hardware you should expect small irregularties in the
simulation as simulates with worse granularity.
*/
/*!
    \qmlproperty ParticleSystem QtQuick.Particles::ParticleAffector::system
    This is the system which will be affected by the element.
    If the ParticleAffector is a direct child of a ParticleSystem, it will automatically be associated with
   it.
*/
/*!
    \qmlproperty list<string> QtQuick.Particles::ParticleAffector::groups
    Which logical particle groups will be affected.

    If empty, it will affect all particles.
*/
/*!
    \qmlproperty list<string> QtQuick.Particles::ParticleAffector::whenCollidingWith
    If any logical particle groups are specified here, then the particle affector
    will only be triggered if the particle being examined intersects with
    a particle of one of these groups.

    This is different from the groups property. The groups property selects which
    particles might be examined, and if they meet other criteria (including being
    within the bounds of the ParticleAffector, modified by shape) then they will be tested
    again to see if they intersect with a particles from one of the particle groups
    in whenCollidingWith.

    By default, no groups are specified.
*/
/*!
    \qmlproperty bool QtQuick.Particles::ParticleAffector::enabled
    If enabled is set to false, this affector will not affect any particles.

    Usually this is used to conditionally turn an affector on or off.

    Default value is true.
*/
/*!
    \qmlproperty bool QtQuick.Particles::ParticleAffector::once
    If once is set to true, this affector will only affect each particle
    once in their lifetimes. If the affector normally simulates a continuous
    effect over time, then it will simulate the effect of one second of time
    the one instant it affects the particle.

    Default value is false.
*/
/*!
    \qmlproperty Shape QtQuick.Particles::ParticleAffector::shape
    If a size has been defined, the shape property can be used to affect a
    non-rectangular area.
*/
/*!
    \qmlsignal QtQuick.Particles::ParticleAffector::affected(real x, real y)

    This signal is emitted when a particle is selected to be affected. It will not be emitted
    if a particle is considered by the ParticleAffector but not actually altered in any way.

    In the special case where an ParticleAffector has no possible effect (e.g. ParticleAffector {}), this signal
    will be emitted for all particles being considered if you connect to it. This allows you to
    execute arbitrary code in response to particles (use the ParticleAffector::onAffectParticles
    signal handler if you want to execute code which affects the particles
    themselves). As this executes JavaScript code per particle, it is not recommended to use this
    signal with a high-volume particle system.

    (\a {x}, \a {y}) is the particle's current position.
*/
QQuickParticleAffector::QQuickParticleAffector(QQuickItem *parent) :
    QQuickItem(parent), m_needsReset(false), m_ignoresTime(false), m_onceOff(false), m_enabled(true)
    , m_system(nullptr), m_updateIntSet(false), m_shape(new QQuickParticleExtruder(this))
{
}

bool QQuickParticleAffector::isAffectedConnected()
{
    IS_SIGNAL_CONNECTED(this, QQuickParticleAffector, affected, (qreal,qreal));
}


void QQuickParticleAffector::componentComplete()
{
    if (!m_system && qobject_cast<QQuickParticleSystem*>(parentItem()))
        setSystem(qobject_cast<QQuickParticleSystem*>(parentItem()));
    QQuickItem::componentComplete();
}

bool QQuickParticleAffector::activeGroup(int g) {
    if (!m_system)
        return false;

    if (m_updateIntSet){ //This can occur before group ids are properly assigned, but that resets the flag
        m_groupIds.clear();
        foreach (const QString &p, m_groups)
            m_groupIds << m_system->groupIds[p];
        m_updateIntSet = false;
    }
    return m_groupIds.isEmpty() || m_groupIds.contains(g);
}

bool QQuickParticleAffector::shouldAffect(QQuickParticleData* d)
{
    if (!d)
        return false;
    if (!m_system)
        return false;

    if (activeGroup(d->groupId)){
        if ((m_onceOff && m_onceOffed.contains(std::make_pair(d->groupId, d->index)))
                || !d->stillAlive(m_system))
            return false;
        //Need to have previous location for affected anyways
        if (width() == 0 || height() == 0
                || m_shape->contains(QRectF(m_offset.x(), m_offset.y(), width(), height()), QPointF(d->curX(m_system), d->curY(m_system)))){
            if (m_whenCollidingWith.isEmpty() || isColliding(d)){
                return true;
            }
        }
    }
    return false;

}

void QQuickParticleAffector::postAffect(QQuickParticleData* d)
{
    if (!m_system)
        return;

    m_system->needsReset << d;
    if (m_onceOff)
        m_onceOffed << std::make_pair(d->groupId, d->index);
    if (isAffectedConnected())
        emit affected(d->curX(m_system), d->curY(m_system));
}

const qreal QQuickParticleAffector::simulationDelta = 0.020;
const qreal QQuickParticleAffector::simulationCutoff = 1.000;//If this goes above 1.0, then m_once behaviour needs special codepath

void QQuickParticleAffector::affectSystem(qreal dt)
{
    if (!m_enabled)
        return;
    if (!m_system)
        return;

    //If not reimplemented, calls affectParticle per particle
    //But only on particles in targeted system/area
    updateOffsets();//### Needed if an ancestor is transformed.
    if (m_onceOff)
        dt = 1.0;
    for (QQuickParticleGroupData* gd : std::as_const(m_system->groupData)) {
        if (activeGroup(gd->index)) {
            for (QQuickParticleData* d : std::as_const(gd->data)) {
                if (shouldAffect(d)) {
                    bool affected = false;
                    qreal myDt = dt;
                    if (!m_ignoresTime && myDt < simulationCutoff) {
                        int realTime = m_system->timeInt;
                        m_system->timeInt -= myDt * 1000.0;
                        while (myDt > simulationDelta) {
                            m_system->timeInt += simulationDelta * 1000.0;
                            if (d->alive(m_system))//Only affect during the parts it was alive for
                                affected = affectParticle(d, simulationDelta) || affected;
                            myDt -= simulationDelta;
                        }
                        m_system->timeInt = realTime;
                    }
                    if (myDt > 0.0)
                        affected = affectParticle(d, myDt) || affected;
                    if (affected)
                        postAffect(d);
                }
            }
        }
    }
}

bool QQuickParticleAffector::affectParticle(QQuickParticleData *, qreal )
{
    return true;
}

void QQuickParticleAffector::reset(QQuickParticleData* pd)
{//TODO: This, among other ones, should be restructured so they don't all need to remember to call the superclass
    if (m_onceOff)
        if (activeGroup(pd->groupId))
            m_onceOffed.remove(std::make_pair(pd->groupId, pd->index));
}

void QQuickParticleAffector::updateOffsets()
{
    if (m_system)
        m_offset = m_system->mapFromItem(this, QPointF(0, 0));
}

bool QQuickParticleAffector::isColliding(QQuickParticleData *d) const
{
    if (!m_system)
        return false;

    qreal myCurX = d->curX(m_system);
    qreal myCurY = d->curY(m_system);
    qreal myCurSize = d->curSize(m_system) / 2;
    foreach (const QString &group, m_whenCollidingWith){
        foreach (QQuickParticleData* other, m_system->groupData[m_system->groupIds[group]]->data){
            if (!other->stillAlive(m_system))
                continue;
            qreal otherCurX = other->curX(m_system);
            qreal otherCurY = other->curY(m_system);
            qreal otherCurSize = other->curSize(m_system) / 2;
            if ((myCurX + myCurSize > otherCurX - otherCurSize
                 && myCurX - myCurSize < otherCurX + otherCurSize)
                 && (myCurY + myCurSize > otherCurY - otherCurSize
                     && myCurY - myCurSize < otherCurY + otherCurSize))
                return true;
        }
    }
    return false;
}

QT_END_NAMESPACE

#include "moc_qquickparticleaffector_p.cpp"
