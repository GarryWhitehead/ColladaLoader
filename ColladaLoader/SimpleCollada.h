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

	struct FaceInfo
	{
		std::string name;
		uint32_t count;
		std::string material;
	};

	struct GeometryInfo 
	{
		MeshInfo vertex;
		MeshInfo normal;
		MeshInfo texcoord;

		std::vector<FaceInfo> face;
	};

	struct GeometryData
	{
		std::vector<glm::vec3> position;
		std::vector<glm::vec3> normal;
		std::vector<glm::vec3> texcoord;
		std::vector< std::vector<uint32_t> > face;
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

	struct MaterialInfo
	{
		std::string name;
		glm::vec4 ambient;
		glm::vec4 specular;
		glm::vec4 color;
		float shininess;
		float transparency;
	};

	struct Bone
	{
		std::string sid;
		std::string name;
		glm::mat4 transform;
	};

	struct BoneInfo
	{
		Bone parent;
		std::vector<Bone> children;
	};
	
	struct SkinningInfo
	{
		glm::mat4 bindShape;
		std::vector<glm::mat4> invBind;
		std::vector<uint32_t> vertCounts;
		std::vector<uint32_t> vertIndices;
		std::vector<std::string> joints;
		std::vector<float> weights;
	};

	SimpleCollada();
	~SimpleCollada();

	bool OpenColladaFile(std::string filename);
	bool ImportColladaData();
	void ImportLibraryAnimations();
	void ImportLibraryImages();
	void ImportGeometryData();
	void ImportLibraryMaterials();
	void ImportLibraryVisualScenes();
	void ImportLibraryControllers();
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
	std::vector<std::string> m_libraryImageData;
	std::vector<MaterialInfo> m_materialData;
	std::vector<BoneInfo> m_boneData;
	SkinningInfo m_skinData;
};



