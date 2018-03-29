#include "SimpleCollada.h"



SimpleCollada::SimpleCollada()
{
}


SimpleCollada::~SimpleCollada()
{
}

bool SimpleCollada::OpenColladaFile(std::string filename)
{
	m_xmlParse = new XMLparse();
	m_xmlParse->LoadXMLDocument(filename);

	if (m_xmlParse->ReportErrors() == ErrorFlags::XMLP_ERROR_UNABLE_TO_OPEN_FILE) {

		return false;
	}

	return true;
}

bool SimpleCollada::ImportColladaData()
{
	// start with library animation data
	ImportLibraryAnimations();

	// Geomtry data
	ImportGeometryData();

	// material paths
	ImportLibraryImages();

	// material data
	ImportLibraryMaterials();

	// skeleton data
	ImportLibraryVisualScenes();

	// skinning data
	ImportLibraryControllers();

	return true;
}

void SimpleCollada::ImportLibraryAnimations()
{
	XMLPbuffer buffer;

	buffer = m_xmlParse->ReadTreeIntoBuffer("library_animations");
	if (m_xmlParse->ReportErrors() == ErrorFlags::XMLP_INCORRECT_FILE_FORMAT) {

		return;
	}

	// before stating, evaluate the type of transform used - X translation, thus only one float, or full transform, then using 4x4 matrix
	// This is found in the "technique_common" node in the child node param name or by checking the stride in accessor source
	uint32_t index = 0;
	index = m_xmlParse->FindElementInBuffer("technique_common", buffer, index);
	std::string type = m_xmlParse->ReadElementDataString("type", buffer, index + 2);		// making the assumpton that all data is laid out the same
	m_transformType = (type == "float") ? TransformType::TRANSFORM_FLOAT : TransformType::TRANSFORM_4X4;

	index = 0;
	while (index < buffer.size()) {

		index = m_xmlParse->FindElementInBuffer("sampler id", buffer, index);
		if (index != UINT32_MAX) {

			// there are three types of input - INPUT which gives timing info; OUPUT - which gives the transform data at that particlaur time point
			// and INTERPOLATION - which gives the type of intrepolation to use between values (usually LINEAR)
			std::string  semantic;
			InputSemanticInfo semanticInfo;

			++index;
			for (int c = 0; c < 3; ++c) {

				semantic = m_xmlParse->ReadElementDataString("input semantic", buffer, index);
				if (!semantic.empty()) {

					if (semantic == "INPUT") {
						semanticInfo.input = m_xmlParse->ReadElementDataString("source", buffer, index++);
					}
					else if (semantic == "OUTPUT") {
						semanticInfo.output = m_xmlParse->ReadElementDataString("source", buffer, index++);
					}
					else if (semantic == "INTERPOLATION") {
						semanticInfo.interpolation = m_xmlParse->ReadElementDataString("source", buffer, index++);
					}
				}
			}
			m_semanticData.push_back(semanticInfo);
		}
	}

	if (m_transformType == TransformType::TRANSFORM_4X4) {

		GetAnimationData4X4Transform(buffer);
	}
	else {
		GetAnimationDataFloatTransform(buffer);
	}

}

void SimpleCollada::GetAnimationDataFloatTransform(XMLPbuffer &buffer)
{
	// now we have the input semantic info - we can now read the corresponding data
	uint32_t index = 0;
	uint32_t semanticIndex = 0;

	while (index < buffer.size()) {
		
		LibraryAnimationFloatInfo animInfo;

		for (int c = 0; c < 3; ++c) {

			index = m_xmlParse->FindElementInBuffer("source id", buffer, index);
			if (index == UINT32_MAX) {
				return;
			}

			std::string id = m_xmlParse->ReadElementDataString("source id", buffer, index++);

			uint32_t arrayCount = 0;
			if (id == m_semanticData[semanticIndex].input) {

				arrayCount = GetFloatArrayInfo(m_semanticData[semanticIndex].input, buffer, index, semanticIndex);
				index = m_xmlParse->ReadElementArray<float>(buffer, animInfo.time, index, arrayCount);
			}
			else if (id == m_semanticData[semanticIndex].output) {

				arrayCount = GetFloatArrayInfo(m_semanticData[semanticIndex].output, buffer, index, semanticIndex);
				index = m_xmlParse->ReadElementArray<float>(buffer, animInfo.transform, index, arrayCount);
			}
		}

		m_libraryAnimationFloatData.push_back(animInfo);
		++semanticIndex;
	}
}

void SimpleCollada::GetAnimationData4X4Transform(XMLPbuffer &buffer)
{
	// now we have the input semantic info - we can now read the corresponding data
	uint32_t index = 0;
	uint32_t semanticIndex = 0;

	LibraryAnimation4X4Info animInfo;

	while (index < buffer.size()) {

		for (int c = 0; c < 3; ++c) {

			index = m_xmlParse->FindElementInBuffer("source id", buffer, index);
			if (index == UINT32_MAX) {
				return;
			}

			std::string id = m_xmlParse->ReadElementDataString("source id", buffer, index++);

			uint32_t arrayCount = 0;
			if (id == m_semanticData[semanticIndex].input) {

				arrayCount = GetFloatArrayInfo(m_semanticData[semanticIndex].input, buffer, index, semanticIndex);
				index = m_xmlParse->ReadElementArray<float>(buffer, animInfo.time, index, arrayCount);
			}
			else if (id == m_semanticData[semanticIndex].output) {

				arrayCount = GetFloatArrayInfo(m_semanticData[semanticIndex].output, buffer, index, semanticIndex);
				index = m_xmlParse->ReadElementArrayMatrix(buffer, animInfo.transform, index, arrayCount);
			}
		}

		m_libraryAnimation4x4Data.push_back(animInfo);
		++semanticIndex;
	}
}

uint32_t SimpleCollada::GetFloatArrayInfo(std::string id, XMLPbuffer& buffer, uint32_t index, uint32_t semanticIndex)
{
	// start by ensuring that this is a float array
	std::string arrayId = m_xmlParse->ReadElementDataString("float_array id", buffer, index);
	if (arrayId != id + "-array") {
		return 0;
	}

	uint32_t arrayCount = m_xmlParse->ReadElementDataInt("count", buffer, index);
	return arrayCount;
}

void SimpleCollada::ImportGeometryData()
{
	XMLPbuffer buffer;

	// begin by reading geomtry data into a buffer
	buffer = m_xmlParse->ReadTreeIntoBuffer("library_geometries");
	if (m_xmlParse->ReportErrors() == ErrorFlags::XMLP_INCORRECT_FILE_FORMAT) {

		return;
	}

	std::string semantic;
	uint32_t index = 0;

	// for some reason, some exporters use interchange the word vertices and position for ids, so check whether this is the case
	index = m_xmlParse->FindElementInBuffer("vertices id", buffer, index);
	++index;
	semantic = m_xmlParse->ReadElementDataString("input semantic", buffer, index);
	if (semantic == "POSITION") {

		m_geometryInfo.vertex.id = m_xmlParse->ReadElementDataString("source", buffer, index);
	}

	// Get vertex information before importing data
	index = m_xmlParse->FindElementInBuffer("/vertices", buffer, index++);
	
	// TODO - allow for multiple meshes
	for (int c = 0; c < 3; ++c) {

		semantic = m_xmlParse->ReadElementDataString("input semantic", buffer, index);
		if (!semantic.empty()) {

			if (semantic == "VERTEX") {

				if (m_geometryInfo.vertex.id.empty()) {
					m_geometryInfo.vertex.id = m_xmlParse->ReadElementDataString("source", buffer, index);
				}
				m_geometryInfo.vertex.offset = m_xmlParse->ReadElementDataInt("offset", buffer, index++);
			}
			else if (semantic == "NORMAL") {
				m_geometryInfo.normal.id = m_xmlParse->ReadElementDataString("source", buffer, index);
				m_geometryInfo.normal.offset = m_xmlParse->ReadElementDataInt("offset", buffer, index++);
			}
			else if (semantic == "TEXCOORD") {
				m_geometryInfo.texcoord.id = m_xmlParse->ReadElementDataString("source", buffer, index);
				m_geometryInfo.texcoord.offset = m_xmlParse->ReadElementDataInt("offset", buffer, index++);
			}
		}
	}

	index = 0;
	uint32_t count = 0;
	// Read data into buffers - TODO : check counts and stride values of each data set

	for (int c = 0; c < 3; ++c) {
		
		index = m_xmlParse->FindElementInBuffer("source id", buffer, index);
		if (index == UINT32_MAX) {
			return;
		}
		++index;
		std::string sourceId = m_xmlParse->ReadElementDataString("float_array id", buffer, index);
		if (sourceId == m_geometryInfo.vertex.id + "-array") {

			count = m_xmlParse->ReadElementDataInt("count", buffer, index);
			index = m_xmlParse->ReadElementArrayVec3(buffer, m_geomtryData.position, index, count);
		}	
		else if (sourceId == m_geometryInfo.normal.id + "-array") {

			count = m_xmlParse->ReadElementDataInt("count", buffer, index);
			index = m_xmlParse->ReadElementArrayVec3(buffer, m_geomtryData.normal, index, count);
		}	
		else if (sourceId == m_geometryInfo.texcoord.id + "-array") {

			count = m_xmlParse->ReadElementDataInt("count", buffer, index);
			index = m_xmlParse->ReadElementArrayVec3(buffer, m_geomtryData.texcoord, index, count);
		}
	}

	// And finally the indices data
	while (index < buffer.size()) {

		index = m_xmlParse->FindElementInBuffer("triangles", buffer, index);
		if (index == UINT32_MAX) {
			return;
		}

		FaceInfo face;
		face.name = m_xmlParse->ReadElementDataString("material", buffer, index);
		face.count = m_xmlParse->ReadElementDataInt("count", buffer, index);
		m_geometryInfo.face.push_back(face);

		index += 4;
		std::vector<uint32_t> faceData;
		index = m_xmlParse->ReadElementArray<uint32_t>(buffer, faceData, index, face.count);
	}
}

void SimpleCollada::ImportLibraryImages()
{
	XMLPbuffer buffer;

	// begin by reading geomtry data into a buffer
	buffer = m_xmlParse->ReadTreeIntoBuffer("library_images");
	if (m_xmlParse->ReportErrors() == ErrorFlags::XMLP_INCORRECT_FILE_FORMAT) {

		return;
	}

	uint32_t index = 0;
	while (index < buffer.size()) {

		index = m_xmlParse->FindElementInBuffer("image", buffer, index);
		if (index == UINT32_MAX) {
			return;
		}
		std::string filename = m_xmlParse->ReadElementDataString("id", buffer, index++);
		m_libraryImageData.push_back(filename);
	}
	
}

void SimpleCollada::ImportLibraryMaterials()
{
	// first link materail id with face and get a count of the number of materials

	XMLPbuffer buffer;

	buffer = m_xmlParse->ReadTreeIntoBuffer("library_materials");
	if (m_xmlParse->ReportErrors() == ErrorFlags::XMLP_INCORRECT_FILE_FORMAT) {

		return;
	}

	uint32_t materialCount = 0;
	uint32_t index = 0;

	while (index < buffer.size()) {

		index = m_xmlParse->FindElementInBuffer("material", buffer, index);
		if (index == UINT32_MAX) {
			break;
		}
		std::string id = m_xmlParse->ReadElementDataString("id", buffer, index);
		std::string matName = m_xmlParse->ReadElementDataString("name", buffer, index++);

		for (auto& face : m_geometryInfo.face) {

			if (id == face.name) {
				face.material = matName;
				break;
			}
		}
		++materialCount;
	}

	// TODO: not using a lot of the information from the file. Will add when needed.
	// Now import the material data
	index = 0;
	XMLPbuffer effectsBuffer;
	effectsBuffer = m_xmlParse->ReadTreeIntoBuffer("library_effects");
	if (m_xmlParse->ReportErrors() == ErrorFlags::XMLP_INCORRECT_FILE_FORMAT) {

		return;
	}

	for(int c = 0; c < materialCount; ++c) {
	
		MaterialInfo material;

		index = m_xmlParse->FindElementInBuffer("effect", effectsBuffer, index);
		material.name = m_xmlParse->ReadElementDataString("name", effectsBuffer, index);
		index = m_xmlParse->FindElementInBuffer("ambient", effectsBuffer, index);
		material.ambient = m_xmlParse->ReadElementDataVec<glm::vec4>("color", effectsBuffer, ++index);
		index = m_xmlParse->FindElementInBuffer("specular", effectsBuffer, index);
		material.specular = m_xmlParse->ReadElementDataVec<glm::vec4>("color", effectsBuffer, ++index);
		index = m_xmlParse->FindElementInBuffer("shininess", effectsBuffer, index);
		material.shininess = m_xmlParse->ReadElementDataFloat("float", effectsBuffer, ++index);
		index = m_xmlParse->FindElementInBuffer("transparency", effectsBuffer, index);
		material.transparency = m_xmlParse->ReadElementDataFloat("float", effectsBuffer, ++index);
		m_materialData.push_back(material);
	}
}

void SimpleCollada::ImportLibraryVisualScenes()
{
	XMLPbuffer buffer;

	buffer = m_xmlParse->ReadTreeIntoBuffer("library_visual_scenes");
	if (m_xmlParse->ReportErrors() == ErrorFlags::XMLP_INCORRECT_FILE_FORMAT) {

		return;
	}

	// joint data can be exported as multiple skeletons which is found in <instance_controller>
	// for now, this data is ignored and the parent is the first joint within a node section and the rest are children
	// also, "NODE" types are ignored for now and only "JOINT" types are used
	uint32_t index = 0;

	index = m_xmlParse->FindElementInBuffer("instance_controller", buffer, index);
	

	while (index < buffer.size()) {

		BoneInfo bone;
		index = m_xmlParse->FindElementInBuffer("node", buffer, index);
		if (index == UINT32_MAX) {
			break;						// no more nodes, so finish
		}

		std::string type = m_xmlParse->ReadElementDataString("type", buffer, index);
		
		if (type == "NODE") {		// ignore "NODE" types
			index += 2;
			continue;
		}
		else {						// assume the type to be a "JOINT"

			bone.parent.sid = m_xmlParse->ReadElementDataString("sid", buffer, index);
			bone.parent.name = m_xmlParse->ReadElementDataString("name", buffer, index++);
			bone.parent.transform = m_xmlParse->ReadElementDataMatrix("matrix", buffer, index++);

			while (m_xmlParse->CheckElement("node", buffer, index)) {

				std::string type = m_xmlParse->ReadElementDataString("type", buffer, index);
				if (type == "NODE") {		// ignore "NODE" types
					index += 2;
					continue;
				}

				Bone childBone;
				childBone.sid = m_xmlParse->ReadElementDataString("sid", buffer, index);
				childBone.name = m_xmlParse->ReadElementDataString("name", buffer, index++);
				childBone.transform = m_xmlParse->ReadElementDataMatrix("matrix", buffer, index++);
				bone.children.push_back(childBone);
			}
		}
		m_boneData.push_back(bone);
	}
}

void SimpleCollada::ImportLibraryControllers()
{
	XMLPbuffer buffer;

	buffer = m_xmlParse->ReadTreeIntoBuffer("library_controllers");
	if (m_xmlParse->ReportErrors() == ErrorFlags::XMLP_INCORRECT_FILE_FORMAT) {

		return;
	}

	uint32_t index = 0;

	index = m_xmlParse->FindElementInBuffer("joints", buffer, index);
	index += 2;
	std::string invBindSource = m_xmlParse->ReadElementDataString("source", buffer, index);

	index = m_xmlParse->FindElementInBuffer("vertex_weights", buffer, index);
	uint32_t vCount = m_xmlParse->ReadElementDataInt("count", buffer, index);
	++index;

	std::string jointSource, weightSource;

	for (int c = 0; c < 2; ++c) {
		std::string semantic = m_xmlParse->ReadElementDataString("semantic", buffer, index);

		if (semantic == "JOINT") {
			jointSource = m_xmlParse->ReadElementDataString("source", buffer, index++);
		}
		else if (semantic == "WEIGHT") {
			weightSource = m_xmlParse->ReadElementDataString("source", buffer, index++);
		}
	}

	// now import vcount (vertex weight count) and v (vertex weight indicies)
	index = m_xmlParse->ReadElementArray<uint32_t>(buffer, m_skinData.vertCounts, index, vCount);
	index = m_xmlParse->ReadElementArray<uint32_t>(buffer, m_skinData.vertIndices, index, vCount);

	// import the joints, inverse binding and weights arrays
	index = 0;
	std::string id;
	uint32_t arrayCount = 0;

	for (int c = 0; c < 3; ++c) {
		
		index = m_xmlParse->FindElementInBuffer("source", buffer, index);
		id = m_xmlParse->ReadElementDataString("id", buffer, index++);
		if (id == jointSource) {

			arrayCount = m_xmlParse->ReadElementDataInt("count", buffer, index);
			index = m_xmlParse->ReadElementArray<std::string>(buffer, m_skinData.joints, index, arrayCount);
		}
		else if (id == invBindSource) {

			arrayCount = m_xmlParse->ReadElementDataInt("count", buffer, index);
			index = m_xmlParse->ReadElementArrayMatrix(buffer, m_skinData.invBind, index, arrayCount);
		}
		else if (id == weightSource) {

			arrayCount = m_xmlParse->ReadElementDataInt("count", buffer, index);
			index = m_xmlParse->ReadElementArray<float>(buffer, m_skinData.weights, index, arrayCount);
		}


	}
}

