#pragma once
#include "XMLparse.h"
#include <vector>
#include <string>

enum class TransformType
{
	TRANSFORM_FLOAT,
	TRANSFORM_4X4
};

class SimpleCollada
{
public:

	struct InputSemanticInfo
	{
		std::string input;
		std::string output;
		std::string interpolation;
	};

	struct MeshInfo
	{
		std::string id;
		uint32_t count;
		uint8_t offset;
	};

	struct GeometryInfo 
	{
		MeshInfo vertex;
		MeshInfo normal;
		MeshInfo texcoord;

		uint32_t indicesCount;
	};

	struct GeometryData
	{
		std::vector<glm::vec3> position;
		std::vector<glm::vec3> normal;
		std::vector<glm::vec3> texcoord;
	};

	struct LibraryAnimationFloatInfo
	{
		std::vector<float> time;
		std::vector<float> transform;
		std::vector<std::string> interpolation;
	};

	struct LibraryAnimation4X4Info
	{
		std::vector<float> time;
		std::vector<glm::mat4> transform;
		std::vector<std::string> interpolation;
	};

	SimpleCollada();
	~SimpleCollada();

	bool OpenColladaFile(std::string filename);
	bool ImportColladaData();
	void ImportLibraryAnimations();
	void ImportGeometryData();
	void GetAnimationDataFloatTransform(XMLPbuffer &buffer);
	void GetAnimationData4X4Transform(XMLPbuffer &buffer);

	uint32_t GetFloatArrayInfo(std::string id, XMLPbuffer& buffer, uint32_t index, uint32_t semanticIndex);

private:

	XMLparse *m_xmlParse;

	std::vector<InputSemanticInfo> m_semanticData;
	GeometryInfo m_geometryInfo;
	GeometryData m_geomtryData;

	TransformType m_transformType;
	std::vector<LibraryAnimationFloatInfo> m_libraryAnimationFloatData;
	std::vector<LibraryAnimation4X4Info> m_libraryAnimation4x4Data;
};



