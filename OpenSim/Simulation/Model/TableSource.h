/* -------------------------------------------------------------------------- *
 *                            OpenSim:  TableSource.h                         *
 * -------------------------------------------------------------------------- *
 * The OpenSim API is a toolkit for musculoskeletal modeling and simulation.  *
 * See http://opensim.stanford.edu and the NOTICE file for more information.  *
 * OpenSim is developed at Stanford University and supported by the US        *
 * National Institutes of Health (U54 GM072970, R24 HD065690) and by DARPA    *
 * through the Warrior Web program.                                           *
 *                                                                            *
 * Copyright (c) 2005-2015 Stanford University and the Authors                *
 * Authors:                                                                   *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.         *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */

#ifndef OPENSIM_TABLE_SOURCE_H_
#define OPENSIM_TABLE_SOURCE_H_

#include "OpenSim/Common/TimeSeriesTable.h"
#include "ModelComponent.h"

namespace OpenSim {

class TimeOutOfRange : public InvalidTimestamp {
    TimeOutOfRange(const std::string& file,
                   size_t line,
                   const std::string& func,
                   double timestamp,
                   double minTimestamp,
                   double maxTimestamp) :
        InvalidTimestamp(file, line, func) {
        std::string msg = "min = " + std::to_string(minTimestamp);
        msg += " max = " + std::to_string(maxTimestamp);
        msg += " timestamp = " + std::to_string(timestamp);

        addMessage(msg);
    }
};


template<typename ET>
class TableSource_ : public ModelComponent {
    OpenSim_DECLARE_CONCRETE_OBJECT_T(TableSource_, ET, ModelComponent);

public:
    typedef SimTK::RowVectorView_<ET> RowVectorView;

    OpenSim_DECLARE_OUTPUT(row, RowVectorView, getRowAtTime, 
                           SimTK::Stage::Time);
    OpenSim_DECLARE_LIST_OUTPUT(column, ET, getColumnAtTime, 
                                SimTK::Stage::Time);


    ET getColumnAtTime(const SimTK::State& state, 
                       const std::string& columnLabel) const {
        OPENSIM_THROW_IF(_table.getNumRows() == 0, EmptyTable);
        const auto& timeCol = _table.getIndependentColumn();
        const auto time = state.getTime();
        OPENSIM_THROW_IF(time < timeCol.front() ||
                         time > timeCol.back(),
                         TimeOutOfRange, 
                         time, timeCol.front(), timeCol.back());

        const auto colInd = _table.getColumnIndex(columnLabel);
        auto lb = std::lower_bound(timeCol.begin(), timeCol.end(), time);
        if(*lb == timeCol.begin())
            return _table.getMatrix().getElt(0, colInd);
        else if(*lb == timeCol.end())
            return _table.getMatrix().getElt(timeCol.size() - 1, colInd);
        else if(*lb == time)
            return _table.getMatrix().getElt(lb - timeCol.begin(), colInd);
        else {
            auto prevTime = *(lb - 1);
            auto nextTime = *lb;
            auto prevElt = _table.getMatrix().getElt(lb - 1 - timeCol.begin(), 
                                                     colInd);
            auto nextElt = _table.getMatrix().getElt(lb - timeCol.begin(), 
                                                     colInd);
            auto elt = ((time - prevTime) / (nextTime - prevTime)) * 
                       (nextElt - prevElt);
            return elt;
        }
    }

    RowVectorView getRowAtTime(const SimTK::State& state) const {
        OPENSIM_THROW_IF(_table.getNumRows() == 0, EmptyTable);
        const auto& timeCol = _table.getIndependentColumn();
        const auto time = state.getTime();
        OPENSIM_THROW_IF(time < timeCol.front() ||
                         time > timeCol.back(),
                         TimeOutOfRange, 
                         time, timeCol.front(), timeCol.back());

        auto lb = std::lower_bound(timeCol.begin(), timeCol.end(), time);
        if(*lb == timeCol.begin())
            return _table.getRowAtIndex(0);
        else if(*lb == timeCol.end())
            return _table.getRowAtIndex(timeCol.size() - 1);
        else if(*lb == time)
            return _table.getRowAtIndex(lb - timeCol.begin());
        else {
            auto prevTime = *(lb - 1);
            auto nextTime = *lb;
            auto prevRow = _table.getRowAtIndex(lb - 1 - timeCol.begin());
            auto nextRow = _table.getRowAtIndex(lb - timeCol.begin());
            auto row = ((time - prevTime) / (nextTime - prevTime)) * 
                       (nextRow - prevRow);
            return row;
        }
    }

private:
    TimeSeriesTable_<ET> _table;

};

typedef TableSource_<SimTK::Real> TableSource;
typedef TableSource_<SimTK::Vec3> TableSourceVec3;

} // namespace Opensim

#endif // OPENSIM_TABLE_SOURCE_H_