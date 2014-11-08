// Copyright (C) 2012, Chris J. Foster and the other authors and contributors
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name of the software's owners nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// (This is the BSD 3-clause license)

#ifndef DISPLAZ_GEOMETRY_H_INCLUDED
#define DISPLAZ_GEOMETRY_H_INCLUDED

#include <memory>

#include "util.h"
#include <QString>
#include <QMetaType>

class QGLShaderProgram;
class TransformState;


/// Estimate of amount of geometry drawn in a frame
///
/// `numVertices` is the number of vertices
/// `moreToDraw` indicates whether the geometry is completely drawn
struct DrawCount
{
    double numVertices;
    bool   moreToDraw;

    DrawCount() : numVertices(0), moreToDraw(false) { }

    DrawCount& operator+=(const DrawCount& rhs)
    {
        numVertices += rhs.numVertices;
        moreToDraw |= rhs.moreToDraw;
        return *this;
    }
};


/// Shared interface for all displaz geometry types
class Geometry : public QObject
{
    Q_OBJECT
    public:
        Geometry();

        virtual ~Geometry() {};

        /// Create geometry of a type which is able to read the given file
        static std::shared_ptr<Geometry> create(QString fileName);

        //--------------------------------------------------
        /// Load geometry from file
        ///
        /// Attempt to load no more than a maximum of maxVertexCount vertices,
        /// simplifying the geometry if possible.
        virtual bool loadFile(QString fileName, size_t maxVertexCount) = 0;

        /// Reload geometry from file
        ///
        /// The default implementation just calls loadFile() with the file name
        virtual bool reloadFile(size_t maxVertexCount);

        //--------------------------------------------------
        /// Draw points using given openGL shader program
        ///
        /// Requires that `pointShaderProg` is already bound and any necessary
        /// uniform variables have been set.
        ///
        /// transState specifies the camera transform, quality specifies the
        /// desired amount of simplification; incrementalDraw is true if this
        /// should be an incremental frame to build on a previous call to
        /// drawPoints which returned true.
        ///
        /// The returned DrawCount should be filled with an estimate of the
        /// actual amount of geometry shaded and whether there's any more to be
        /// drawn.
        virtual DrawCount drawPoints(QGLShaderProgram& pointShaderProg,
                                     const TransformState& transState, double quality,
                                     bool incrementalDraw) const { return DrawCount(); }

        /// Draw edges with the given shader
        virtual void drawEdges(QGLShaderProgram& edgeShaderProg,
                               const TransformState& transState) const {}
        /// Draw faces with the given shader
        virtual void drawFaces(QGLShaderProgram& faceShaderProg,
                               const TransformState& transState) const {}

        /// Return total number of vertices
        virtual size_t pointCount() const = 0;

        /// Estimate the number of vertices which would be shaded when
        /// the draw() functions are called with the given quality settings.
        ///
        /// transState and incrementalDraw are as in drawPoints.
        ///
        /// `drawCounts[i]` should be filled with an estimate of the count of
        /// verts drawn at the given quality `qualities[i]`.  `numEstimates` is
        /// the number of elements in the qualities array.
        virtual void estimateCost(const TransformState& transState,
                                  bool incrementalDraw, const double* qualities,
                                  DrawCount* drawCounts, int numEstimates) const = 0;

        /// Pick a vertex on the geometry given a ray representing a mouse click
        ///
        /// The idea is to choose the vertex closest to what the user meant
        /// when they clicked.  (This is easy enough for meshes - just choose
        /// the first intersection - but is pretty subjective and tricky for
        /// point clouds.)
        ///
        /// The "closest" point is chosen by measuring euclidian distance but
        /// with the ray direction scaled by the amount normalDirectionScale.
        /// This distance is returned in the distance parameter when it is
        /// non-null.
        virtual V3d pickVertex(const V3d& rayOrigin, const V3d& rayDirection,
                               double longitudinalScale, double* distance = 0) const = 0;

        //--------------------------------------------------
        /// Get file name describing the source of the geometry
        const QString& fileName() const { return m_fileName; }

        /// Return offset for geometry coordinate system.
        ///
        /// Naively storing vertices as 32 bit floating point doesn't work for
        /// geographic coordinate systems due to precision issues: a small
        /// object may be located very far from the coordinate system origin.
        const V3d& offset() const { return m_offset; }

        /// Get geometric centroid (centre of mass)
        const V3d& centroid() const { return m_centroid; }

        /// Get axis aligned bounding box containing the geometry
        const Imath::Box3d& boundingBox() const { return m_bbox; }

    signals:
        /// Emitted at the start of a point loading step
        void loadStepStarted(QString stepDescription);
        /// Emitted as progress is made loading points
        void loadProgress(int percentLoaded);

    protected:
        void setFileName(const QString& fileName) { m_fileName = fileName; }
        void setOffset(const V3d& offset) { m_offset = offset; }
        void setCentroid(const V3d& centroid) { m_centroid = centroid; }
        void setBoundingBox(const Imath::Box3d& bbox) { m_bbox = bbox; }

    private:
        QString m_fileName;
        V3d m_offset;
        V3d m_centroid;
        Imath::Box3d m_bbox;
};


Q_DECLARE_METATYPE(std::shared_ptr<Geometry>)


#endif // DISPLAZ_GEOMETRY_H_INCLUDED
