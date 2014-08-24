#pragma once

#include "ofMain.h"

#include "ofxAlembicType.h"

#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreHDF5/All.h>

namespace ofxAlembic
{
class Writer;

}

class ofxAlembic::Writer
{
public:

	~Writer() { close(); }

	bool open(const string& path, float fps = 30);
	void close();

	void addPoints(const string& path, const Points& points);
	void addPolyMesh(const string& path, const PolyMesh& polymesh);
	void addCurves(const string& path, const Curves& curves);
    void addCamera(const string& path, const Camera& camera);

	void setTime(float time);
	float getTime() const { return current_time; }

	void rewind();

	void flashFrame();

protected:

	map<string, Alembic::AbcGeom::OObject*> object_map;
	Alembic::AbcGeom::OArchive archive;

	float inv_fps;
	float current_time;

	template <typename T>
	T& getObject(const string& path, const string* parent = NULL)
	{
		using namespace Alembic::AbcGeom;

		string p = path;
		if (p.size() && p[0] == '/')
		{
			p = p.substr(1, p.size() - 1);

			if (p[0] == '/')
			{
				ofLogError("ofxAlembic::Writer") << "invalid path: '" << path << "'";
				throw;
			}
		}

		map<string, OObject*>::iterator it = object_map.find(p);
		map<string, OObject*>::iterator itParent = object_map.end();
        
        if (parent) {
            itParent = object_map.find(parent->c_str());
            if (itParent == object_map.end()) {
                ofLogError("ofxAlembic::Writer") << "parent not found.";
                throw;
            }
        }

		if (it == object_map.end())
		{
			TimeSampling Ts(inv_fps, current_time);
			Alembic::Util::uint32_t tsidx = archive.addTimeSampling(Ts);

            T *t = NULL;
            if (parent) {
                t = new T(*object_map[(*parent)], p);
            } else {
                t = new T(archive.getTop(), p);
            }
			object_map[p] = t;

			t->getSchema().setTimeSampling(tsidx);
		}

		return *((T*)object_map[p]);
	}

};