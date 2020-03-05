#include "ofxAlembicType.h"

using namespace ofxAlembic;
using namespace Alembic::AbcGeom;

#pragma mark - XForm

XForm::XForm(const glm::mat4& matrix)
	: mat(toAbc(matrix))
{
	
}

void XForm::draw()
{
	ofDrawAxis(10);
}

// via. https://github.com/satoruhiga/ofxEulerAngles/

inline static ofVec3f toEulerXYZ(const ofMatrix4x4 &m)
{
	ofVec3f v;
	
	float &thetaX = v.x;
	float &thetaY = v.y;
	float &thetaZ = v.z;
	
	const float &r00 = m(0, 0);
	const float &r01 = m(1, 0);
	const float &r02 = m(2, 0);
	
	const float &r10 = m(0, 1);
	const float &r11 = m(1, 1);
	const float &r12 = m(2, 1);
	
	const float &r20 = m(0, 2);
	const float &r21 = m(1, 2);
	const float &r22 = m(2, 2);
	
	if (r02 < +1)
	{
		if (r02 > -1)
		{
			thetaY = asinf(r02);
			thetaX = atan2f(-r12, r22);
			thetaZ = atan2f(-r01, r00);
		}
		else     // r02 = -1
		{
			// Not a unique solution: thetaZ - thetaX = atan2f(r10,r11)
			thetaY = -PI / 2;
			thetaX = -atan2f(r10, r11);
			thetaZ = 0;
		}
	}
	else // r02 = +1
	{
		// Not a unique solution: thetaZ + thetaX = atan2f(r10,r11)
		thetaY = +PI / 2;
		thetaX = atan2f(r10, r11);
		thetaZ = 0;
	}
	
	thetaX = ofRadToDeg(thetaX);
	thetaY = ofRadToDeg(thetaY);
	thetaZ = ofRadToDeg(thetaZ);
	
	return v;
}

void XForm::get(Alembic::AbcGeom::OXformSchema &schema) const
{
	XformSample samp;
	
	XformOp transop(kTranslateOperation, kTranslateHint);
	XformOp rotatop(kRotateOperation, kRotateHint);
	XformOp scaleop(kScaleOperation, kScaleHint);

	ofMatrix4x4 m = toOf(mat);
	ofVec3f t, s;
	ofQuaternion R, so;
	m.decompose(t, R, s, so);
	
	ofVec3f xyz = toEulerXYZ(R);
	
	samp.addOp(transop, toAbc(t));
	samp.addOp(rotatop, V3d(1.0, 0.0, 0.0), xyz.x);
	samp.addOp(rotatop, V3d(0.0, 1.0, 0.0), xyz.y);
	samp.addOp(rotatop, V3d(0.0, 0.0, 1.0), xyz.z);
	samp.addOp(scaleop, toAbc(s));

	schema.set(samp);
}

void XForm::set(Alembic::AbcGeom::IXformSchema &schema, float time)
{
	ISampleSelector ss(time, ISampleSelector::kNearIndex);
	
	const M44d& m = schema.getValue(ss).getMatrix();
	const double *src = m.getValue();
	float *dst = mat.getValue();
	
	for (int i = 0; i < 16; i++)
		dst[i] = src[i];
}

#pragma mark - Points

Points::Points(const vector<glm::vec3>& ofpoints)
{
	for (int i = 0; i < ofpoints.size(); i++)
	{
		points.push_back(ofpoints[i]);
	}
}

void Points::get(OPointsSchema &schema) const
{
	int num = points.size();

	vector<V3f> positions(num);
	vector<uint64_t> ids(num);

	for (int i = 0; i < num; i++)
	{
		positions[i] = toAbc(points[i].pos);
		ids[i] = points[i].id;
	}

	OPointsSchema::Sample sample((P3fArraySample(positions)),
								 UInt64ArraySample(ids));
	schema.set(sample);
}

void Points::set(IPointsSchema &schema, float time)
{
	ISampleSelector ss(time, ISampleSelector::kNearIndex);
	IPointsSchema::Sample sample;
	schema.get(sample, ss);

	P3fArraySamplePtr m_positions = sample.getPositions();

	size_t num_points = m_positions->size();
	const V3f *src = m_positions->get();
	V3f dst;

	points.resize(num_points);

	for (int i = 0; i < num_points; i++)
	{
		const V3f& v = src[i];
		points[i].pos = glm::vec3(v.x, v.y, v.z);
	}
}

void Points::draw()
{
	ofVboMesh vbomesh;
	for (auto& p : points)
		vbomesh.addVertex(p.pos);
	vbomesh.drawVertices();
}

#pragma mark - PolyMesh

void PolyMesh::get(OPolyMeshSchema &schema) const
{
	vector<V3f> positions;
	vector<int32_t> indexes;
	vector<int32_t> counts;
	vector<V2f> uvs;
	vector<N3f> norms;

	OV2fGeomParam::Sample uv_sample;
	ON3fGeomParam::Sample norm_sample;

	if (mesh.getNumIndices())
	{
		const int num_samples = mesh.getNumIndices();

		const vector<ofIndexType>& idx = mesh.getIndices();

		{
			indexes.resize(num_samples);
			for (int i = 0; i < num_samples; i++)
				indexes[i] = i;
		}

		{
			const std::vector<glm::vec3>& verts = mesh.getVertices();
			positions.resize(num_samples);

			for (int i = 0; i < num_samples; i++)
				positions[i] = toAbc(verts[idx[i]]);
		}

		if (mesh.getNumTexCoords() == mesh.getNumVertices())
		{
			const std::vector<glm::vec2> &v = mesh.getTexCoords();

			uvs.resize(num_samples);
			for (int i = 0; i < num_samples; i++)
				uvs[i] = toAbc(v[idx[i]]);
		}
		assert(uvs.size() == 0 || uvs.size() == num_samples);

		if (mesh.getNumNormals() == mesh.getNumVertices())
		{
			const std::vector<glm::vec3> &v = mesh.getNormals();

			norms.resize(num_samples);
			for (int i = 0; i < num_samples; i++)
				norms[i] = toAbc(glm::normalize(v[idx[i]]) * -1);
		}
		assert(norms.size() == 0 || norms.size() == num_samples);
	}
	else
	{
		const int num_samples = mesh.getNumVertices();

		{
			indexes.resize(num_samples);
			for (int i = 0; i < num_samples; i++)
				indexes[i] = i;
		}

		{
			const std::vector<glm::vec3>& verts = mesh.getVertices();
			positions.resize(num_samples);

			for (int i = 0; i < num_samples; i++)
				positions[i] = toAbc(verts[i]);
		}

		if (mesh.getNumTexCoords() == num_samples)
		{
			const std::vector<glm::vec2> &v = mesh.getTexCoords();

			uvs.resize(num_samples);
			for (int i = 0; i < num_samples; i++)
				uvs[i] = toAbc(v[i]);
		}
		assert(uvs.size() == 0 || uvs.size() == num_samples);

		if (mesh.getNumNormals() == num_samples)
		{
			const std::vector<glm::vec3> &v = mesh.getNormals();
			norms.resize(num_samples);
			for (int i = 0; i < num_samples; i++)
				norms[i] = toAbc(glm::normalize(v[i]) * -1);
		}
		assert(norms.size() == 0 || norms.size() == num_samples);
	}

	// supports only triangles
	{
		int num_tris = indexes.size() / 3;
		counts.resize(num_tris);
		for (int i = 0; i < num_tris; i++)
			counts[i] = 3;
	}

	if (!uvs.empty())
	{
		uv_sample.setScope(kVertexScope);
		uv_sample.setVals(V2fArraySample(uvs));
	}

	if (!norms.empty())
	{
		norm_sample.setScope(kVertexScope);
		norm_sample.setVals(N3fArraySample(norms));
	}

	OPolyMeshSchema::Sample sample((P3fArraySample(positions)),
								   Int32ArraySample(indexes),
								   Int32ArraySample(counts),
								   uv_sample,
								   norm_sample);
	schema.set(sample);
}

void PolyMesh::set(IPolyMeshSchema &schema, float time)
{
	ISampleSelector ss(time, ISampleSelector::kNearIndex);
	IPolyMeshSchema::Sample sample;
	schema.get(sample, ss);
    
	P3fArraySamplePtr m_meshP = sample.getPositions();
	Int32ArraySamplePtr m_meshIndices = sample.getFaceIndices();
	Int32ArraySamplePtr m_meshCounts = sample.getFaceCounts();
    
	mesh.clear();

	size_t numFaces = m_meshCounts->size();
    size_t numIndices = m_meshIndices->size();
	size_t numPoints = m_meshP->size();
	if (numFaces < 1 ||
		numIndices < 1 ||
		numPoints < 1)
	{
		return;
	}

	// TODO: organaize face index
	// make Triangle and Quad class

	typedef Imath::Vec3<unsigned int> Tri;
	typedef std::vector<Tri> TriArray;

	TriArray m_triangles;

    
    
    
    if ( numIndices > 0 ) {
        {
            
            const int32_t* indices = m_meshIndices->get();
            auto & dst = mesh.getIndices();
            dst.resize(numIndices);
            
            auto * dst_ptr = dst.data();
            
            
            size_t faceIndexBegin = 0;
            size_t faceIndexEnd = 0;
            for (size_t face = 0; face < numFaces; ++face)
            {
                faceIndexBegin = faceIndexEnd;
                size_t count = (*m_meshCounts)[face];
                faceIndexEnd = faceIndexBegin + count;
                
                // Check this face is valid
                if (faceIndexEnd > numIndices ||
                    faceIndexEnd < faceIndexBegin)
                {
                    break;
                }
                
                // Make triangles to fill this face.
                if (count >= 3)
                {
                    const auto & v0 = indices[faceIndexBegin + 0];
                    memcpy(dst_ptr++, &v0, sizeof(ofIndexType));
                    
                    const auto & v1 = indices[faceIndexBegin + 1];
                    memcpy(dst_ptr++, &v1, sizeof(ofIndexType));
                    
                    const auto & v2 = indices[faceIndexBegin + 2];
                    memcpy(dst_ptr++, &v2, sizeof(ofIndexType));
                    
                    for (size_t c = 3; c < count; ++c)
                    {
                        const auto & v0 = indices[faceIndexBegin + 0];
                        memcpy(dst_ptr++, &v0, sizeof(ofIndexType));
                        
                        const auto & v1 = indices[faceIndexBegin + c - 1];
                        memcpy(dst_ptr++, &v1, sizeof(ofIndexType));
                        
                        const auto & v2 = indices[faceIndexBegin + c];
                        memcpy(dst_ptr++, &v2, sizeof(ofIndexType));
                    }
                }
            }
            
//            for (int i = 0; i < numIndices; i++)
//            {
//
//                const auto & v0 = indices[i];
//                memcpy(dst_ptr++, &v0, sizeof(ofIndexType));
//            }
        }
        {
            const V3f* points = m_meshP->get();
            std::vector<glm::vec3>& dst = mesh.getVertices();
            dst.resize(numPoints);
            
            glm::vec3* dst_ptr = dst.data();
            
            for (int i = 0; i < dst.size(); i++)
            {
                const V3f& v0 = points[i];
                memcpy(dst_ptr++, &v0.x, sizeof(float) * 3);
            }
        }
        {
            IN3fGeomParam N = schema.getNormalsParam();
            if (N.valid())
            {
                if ( N.isIndexed() ) {
                    N3fArraySamplePtr norm_ptr = N.getExpandedValue(ss).getVals();
                    const N3f* src = norm_ptr->get();
                    size_t numNormals = norm_ptr->size();
                    std::vector<glm::vec3>& dst = mesh.getNormals();
                    int num_verts = mesh.getNumVertices();
                    auto & indices = mesh.getIndices();
                    int num_indices = indices.size();
                    dst.resize(numNormals);
                    
                    glm::vec3* dst_ptr = dst.data();
                    
                    for (int i = 0; i < numNormals; i++)
                    {
                        
                        const N3f& n0 = src[i];
                        //memcpy(dst_ptr++, &n0.x, sizeof(float) * 3);
                        dst[indices[i]].x = n0.x;
                        dst[indices[i]].y = n0.y;
                        dst[indices[i]].z = n0.z;
                    }
                } else {
                    N3fArraySamplePtr norm_ptr = N.getExpandedValue(ss).getVals();
                    const N3f* src = norm_ptr->get();
                    size_t numNormals = norm_ptr->size();
                    std::vector<glm::vec3>& dst = mesh.getNormals();
                    dst.resize(numNormals);
                    
                    glm::vec3* dst_ptr = dst.data();
                    
                    for (int i = 0; i < numNormals; i++)
                    {
                        
                        const N3f& n0 = src[i];
                        memcpy(dst_ptr++, &n0.x, sizeof(float) * 3);
                    }
                }
                
                
            }
        }
        
        {
            IV2fGeomParam UV = schema.getUVsParam();
            if (UV.valid())
            {
                if ( UV.isIndexed() ) {
                    auto value = UV.getExpandedValue(ss);
                    V2fArraySamplePtr uv_ptr = value.getVals();
                    const V2f* src = uv_ptr->get();
                    size_t numUVs = uv_ptr->size();
                    std::vector<glm::vec2>& dst = mesh.getTexCoords();
                    int num_verts = mesh.getNumVertices();
                    auto & indices = mesh.getIndices();
                    int num_indices = indices.size();
                    dst.resize(num_verts);
                    
                    glm::vec2* dst_ptr = dst.data();
                    
                    for (int i = 0; i < numUVs; i++)
                    {
                        const V2f& t0 = src[i];
                        //memcpy(dst_ptr++, &t0.x, sizeof(float) * 2);
                        dst[indices[i]].x = t0.x;
                        dst[indices[i]].y = t0.y;
                    }
                } else {
                    auto value = UV.getExpandedValue(ss);
                    V2fArraySamplePtr uv_ptr = value.getVals();
                    const V2f* src = uv_ptr->get();
                    size_t numUVs = uv_ptr->size();
                    std::vector<glm::vec2>& dst = mesh.getTexCoords();
                    dst.resize(numUVs);
                    
                    glm::vec2* dst_ptr = dst.data();
                    
                    for (int i = 0; i < numUVs; i++)
                    {
                        const V2f& t0 = src[i];
                        memcpy(dst_ptr++, &t0.x, sizeof(float) * 2);
                    }
                }
            }
        }
        { // Color (Custom Attribute)
            const auto & geom_params = schema.getArbGeomParams();
            int num = geom_params.getNumProperties();
            //        cout << num << endl;
            for ( int i=0; i<num; i++ ) {
                const auto & header = geom_params.getPropertyHeader(i);
                auto datatype = header.getDataType();
//                cout << header.getName() << endl;
//                cout << header.getDataType() << endl;
//                cout << header.isArray() << endl;
                string name = header.getName();
                if ( IC3fGeomParam::matches(header) ) {
                    IC3fGeomParam param(geom_params, name);
                    
                    //auto value = param.getIndexedValue(ss);
                    if ( param.isIndexed() ) {
                        C3fArraySamplePtr ptr = param.getExpandedValue(ss).getVals();
                        const C3f* src = ptr->get();
                        size_t numColors = ptr->size();
                        auto & dst = mesh.getColors();
                        int num_verts = mesh.getNumVertices();
                        auto & indices = mesh.getIndices();
                        int num_indices = indices.size();
                        dst.resize(num_verts);
                        
                        auto * dst_ptr = dst.data();
                        
                        for (int i = 0; i < numColors; i++)
                        {
                            const C3f& v0 = src[i];
                            //memcpy(dst_ptr++, &v0.x, sizeof(float) * 3);
                            dst[indices[i]].set(v0.x, v0.y, v0.z);
                        }
                    } else {
                        C3fArraySamplePtr ptr = param.getExpandedValue(ss).getVals();
                        const C3f* src = ptr->get();
                        size_t numColors = ptr->size();
                        auto & dst = mesh.getColors();
                        dst.resize(numColors);
                        
                        auto * dst_ptr = dst.data();
                        
                        for (int i = 0; i < numColors; i++)
                        {
                            const C3f& v0 = src[i];
                            memcpy(dst_ptr++, &v0.x, sizeof(float) * 3);
                        }
                    }
                    
                } else if ( IC4fGeomParam::matches(header) ) {
                    IC4fGeomParam param(geom_params, name);
                    if ( param.isIndexed() ) {
                        C4fArraySamplePtr ptr = param.getExpandedValue(ss).getVals();
                        const C4f* src = ptr->get();
                        size_t numColors = ptr->size();
                        auto & dst = mesh.getColors();
                        int num_verts = mesh.getNumVertices();
                        auto & indices = mesh.getIndices();
                        int num_indices = indices.size();
                        dst.resize(num_verts);
                        
                        auto * dst_ptr = dst.data();
                        
                        for (int i = 0; i < numColors; i++)
                        {
                            
                            const C4f& v0 = src[i];//src[indices[t[0]]];
                            //memcpy(dst_ptr++, &v0.r, sizeof(float) * 4);
                            dst[indices[i]].set(v0.r, v0.g, v0.b, v0.a);
                        }
                    } else {
                        C4fArraySamplePtr ptr = param.getExpandedValue(ss).getVals();
                        const C4f* src = ptr->get();
                        size_t numColors = ptr->size();
                        auto & dst = mesh.getColors();
                        dst.resize(numColors);
                        
                        auto * dst_ptr = dst.data();
                        
                        for (int i = 0; i < numColors; i++)
                        {
                            
                            const C4f& v0 = src[i];//src[indices[t[0]]];
                            memcpy(dst_ptr++, &v0.r, sizeof(float) * 4);
                        }
                    }
                    
                    
                }
            }
        }
        return;
    }
    
}

void PolyMesh::draw()
{
	if (ofGetStyle().bFill)
	{
		mesh.draw();
	}
	else
	{
		mesh.drawWireframe();
	}
}

#pragma mark - Curves

void Curves::get(OCurvesSchema &schema) const
{
	vector<V3f> positions;
	vector<int32_t> num_vertices;

	for (int n = 0; n < curves.size(); n++)
	{
		const ofPolyline &polyline = curves[n];

		for (int i = 0; i < polyline.size(); i++)
		{
			positions.push_back(toAbc(polyline[i]));
		}

		num_vertices.push_back(polyline.size());
	}

	OCurvesSchema::Sample sample((P3fArraySample(positions)),
								 Int32ArraySample(num_vertices),
								 kLinear,
								 kNonPeriodic);
	schema.set(sample);
}

void Curves::set(ICurvesSchema &schema, float time)
{
	ISampleSelector ss(time, ISampleSelector::kNearIndex);
	ICurvesSchema::Sample sample;
	schema.get(sample, ss);

	P3fArraySamplePtr m_positions = sample.getPositions();
	std::size_t m_nCurves = sample.getNumCurves();

	const V3f *src = m_positions->get();
	const Alembic::Util::int32_t *nVertices = sample.getCurvesNumVertices()->get();
	V3f dst;

	curves.resize(m_nCurves);

	for (int i = 0; i < m_nCurves; i++)
	{
		ofPolyline &polyline = curves[i];
		const int num = nVertices[i];

		polyline.clear();

		for (int n = 0; n < num; n++)
		{
			const V3f& v = *src;
			polyline.addVertex(ofVec3f(v.x, v.y, v.z));
			src++;
		}
	}
}

void Curves::draw()
{
	for (int i = 0; i < curves.size(); i++)
	{
		curves[i].draw();
	}
}


#pragma mark - Camera

void Camera::get(OCameraSchema &schema) const
{
	Alembic::AbcGeom::CameraSample sample;
	sample.setHorizontalAperture(this->sample.getHorizontalAperture());
	sample.setVerticalAperture(this->sample.getVerticalAperture());
	sample.setFocalLength(this->sample.getFocalLength());
	schema.set(sample);
}

void Camera::set(ICameraSchema &schema, float time)
{
	ISampleSelector ss(time, ISampleSelector::kNearIndex);
	schema.get(sample, ss);
}

void Camera::updateSample(const ofCamera &camera)
{
	const double aspect = (width == 0 || height == 0) ?
				((double) ofGetViewportHeight()) / ofGetViewportWidth() :
				((double) height) / width;
	const double horizontalAperture = 3.6; // Sensor size in cm
	
	sample.setHorizontalAperture(horizontalAperture);
	sample.setVerticalAperture(horizontalAperture * aspect);
	
	float fovDeg = camera.getFov();
	double focalCm = sample.getVerticalAperture() * 0.5 / tan(ofDegToRad(fovDeg) * 0.5);
	double focalMm = focalCm * 10.0;
	
	sample.setFocalLength(focalMm);
}

void Camera::updateParams(ofCamera &camera, ofMatrix4x4 xform)
{
	float w, h;
	if (width == 0 || height == 0)
	{
		w = ofGetViewportWidth();
		h = ofGetViewportHeight();
	}
	else
	{
		w = width;
		h = height;
	}

	float fovH = sample.getFieldOfView();
	float fovV = ofRadToDeg(2 * atanf(tanf(ofDegToRad(fovH) / 2) * (h / w)));
	camera.setFov(fovV);
	camera.setGlobalPosition(xform.getTranslation());
	camera.setGlobalOrientation(xform.getRotate());

	// TODO: lens offset
}

void Camera::draw()
{
}
