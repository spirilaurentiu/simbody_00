#ifndef SIMTK_MASS_ELEMENT_REP_H_
#define SIMTK_MASS_ELEMENT_REP_H_

/* Copyright (c) 2005-6 Stanford University and Michael Sherman.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including 
 * without limitation the rights to use, copy, modify, merge, publish, 
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**@file
 * Declarations for the *real* MassElement objects. These are opaque to
 * users.
 */

#include "MassElement.h"
#include "FeatureRep.h"

namespace simtk {

/**
 * This is a still-abstract Feature representation, common to all
 * the MassElement features.
 */
class MassElementRep : public FeatureRep {
public:
    MassElementRep(MassElement& m, const std::string& nm) : FeatureRep(m,nm) { }
    // must call initializeStandardSubfeatures() to complete construction.


    const RealMeasure& getMassMeasure() const {
        return RealMeasure::downcast(getSubfeature(massMeasureIndex));
    }
    const StationMeasure& getCentroidMeasure() const {
        return StationMeasure::downcast(getSubfeature(centroidMeasureIndex));
    }

    // virtuals getFeatureTypeName() && clone() still missing

    SIMTK_DOWNCAST(MassElementRep,FeatureRep);

protected:
    // Every MassElement defines some mass-oriented measures.
    virtual void initializeStandardSubfeatures() {
        Feature& mm = addSubfeatureLike(RealMeasure("massMeasure"), "massMeasure");
        Feature& cm = addSubfeatureLike(StationMeasure("centroidMeasure"), "centroidMeasure");
        massMeasureIndex     = mm.getIndexInParent();
        centroidMeasureIndex = cm.getIndexInParent();
    }

private:
    int massMeasureIndex, centroidMeasureIndex;
};

class PointMassElementRep : public MassElementRep {
public:
    PointMassElementRep(PointMassElement& pm, const std::string& nm) 
      : MassElementRep(pm,nm), massIndex(-1) { }
    // must call initializeStandardSubfeatures() to complete construction.

    Placement convertToRequiredPlacementType(const Placement& p) const {
        return p.getRep().castToStationPlacement();
    }

    // some self-placements
    void setMass(const Real& m) {
        updSubfeature(massIndex).place(RealPlacement(m));
    }

    std::string getFeatureTypeName() const { return "PointMassElement"; }
    PlacementType getRequiredPlacementType() const { return StationPlacementType; }
    FeatureRep* clone() const { return new PointMassElementRep(*this); }

    PlacementRep* createFeatureReference(Placement& p, int i) const { 
        PlacementRep* prep=0;
        if (i == -1) 
            prep = new StationFeaturePlacementRep(getMyHandle());
        else if (0<=i && i<3)
            prep = new RealFeaturePlacementRep(getMyHandle(), i);
        if (prep) {
            prep->setMyHandle(p); p.setRep(prep);
            return prep;
        }

        SIMTK_THROW3(Exception::IndexOutOfRangeForFeaturePlacementReference,
            getFullName(), getFeatureTypeName(), i);
        //NOTREACHED
        return 0;
    }

    SIMTK_DOWNCAST2(PointMassElementRep,MassElementRep,FeatureRep);

protected:
    // This will be called from initializeStandardSubfeatures() in MassElement.
    virtual void initializeStandardSubfeatures() {
        MassElementRep::initializeStandardSubfeatures();

        massIndex = addSubfeatureLike(RealParameter("mass"), "mass")
                            .getIndexInParent();

        findUpdSubfeature("massMeasure")->place(*findSubfeature("mass"));
        findUpdSubfeature("centroidMeasure")->place(getMyHandle());
    }

    int massIndex;
};

class CylinderMassElementRep : public MassElementRep {
public:
    CylinderMassElementRep(CylinderMassElement& cm, const std::string& nm) 
      : MassElementRep(cm,nm),
        massIndex(-1), radiusIndex(-1), halfLengthIndex(-1), centerIndex(-1), axisIndex(-1)
    { }
    // must call initializeStandardSubfeatures() to complete construction.

    // no placement for the cylinder as a whole
    Placement convertToRequiredPlacementType(const Placement& p) const {
        return Placement();
    }

    // some self-placements
    void setMass(const Real& m) {
        updSubfeature(massIndex).place(RealPlacement(m));
    }
    void setRadius(const Real& r) {
        updSubfeature(radiusIndex).place(RealPlacement(r));
    }
    void setHalfLength(const Real& h) {
        updSubfeature(halfLengthIndex).place(RealPlacement(h));
    }
    void placeCenter(const Vec3& c) {
        updSubfeature(centerIndex).place(StationPlacement(c));
    }
    void placeAxis(const Vec3& a) {
        updSubfeature(axisIndex).place(DirectionPlacement(a));
    }

    std::string getFeatureTypeName() const { return "CylinderMassElement"; }

    // Cylinder has subfeatures that need Placement, but does not
    // have its own Placement.
    PlacementType getRequiredPlacementType() const { return VoidPlacementType; }
    FeatureRep* clone() const { return new CylinderMassElementRep(*this); }
    PlacementRep* createFeatureReference(Placement&, int) const {
        SIMTK_THROW2(Exception::NoFeatureLevelPlacementForThisKindOfFeature,
            getFullName(), getFeatureTypeName());
        //NOTREACHED
        return 0;
    }

    SIMTK_DOWNCAST2(CylinderMassElementRep,MassElementRep,FeatureRep);
private:
    // This will be called from initializeStandardSubfeatures() in MassElement.
    virtual void initializeStandardSubfeatures() {
        MassElementRep::initializeStandardSubfeatures();

        massIndex       = addSubfeatureLike(RealParameter("mass"),       "mass")
                            .getIndexInParent();
        radiusIndex     = addSubfeatureLike(RealParameter("radius"),     "radius")
                            .getIndexInParent();
        halfLengthIndex = addSubfeatureLike(RealParameter("halfLength"), "halfLength")
                            .getIndexInParent();
        centerIndex     = addSubfeatureLike(Station      ("center"),     "center")
                            .getIndexInParent();
        axisIndex       = addSubfeatureLike(Direction    ("axis"),       "axis")
                            .getIndexInParent();

        findUpdSubfeature("massMeasure")->place(*findSubfeature("mass"));
        findUpdSubfeature("centroidMeasure")->place(*findSubfeature("center"));
    }

    int massIndex, radiusIndex, halfLengthIndex, centerIndex, axisIndex;
};

} // namespace simtk


#endif // SIMTK_MASS_ELEMENT_REP_H_
