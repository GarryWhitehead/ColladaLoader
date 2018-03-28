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

	return true;
}

void SimpleCollada::ImportLibraryAnimations()
{
	XMLPbuffer buffer;

	buffer = m_xmlParse->ReadNodeIntoBuffer("library_animations");
	if (m_xmlParse->ReportErrors() == ErrorFlags::XMLP_INCORRECT_FILE_FORMAT) {

		return;
	}

	// before stating, evaluate the type of transform used - X translation, thus only one float, or full transform, then using 4x4 matrix
	// This is found in the "technique_common" node in the child node param name or by checking the stride in accessor source
	uint32_t index = 0;
	index = m_xmlParse->FindNodeInBuffer("technique_common", buffer, index);
	std::string type = m_xmlParse->ReadChildNodeData("type", buffer, index + 2);		// making the assumpton that all data is laid out the same
	m_transformType = (type == "float") ? TransformType::TRANSFORM_FLOAT : TransformType::TRANSFORM_4X4;

	index = 0;
	while (index < buffer.size()) {

		index = m_xmlParse->FindNodeInBuffer("sampler id", buffer, index);
		if (index != UINT32_MAX) {

			// there are three types of input - INPUT which gives timing info; OUPUT - which gives the transform data at that particlaur time point
			// and INTERPOLATION - which gives the type of intrepolation to use between values (usually LINEAR)
			std::string  semantic;
			InputSemanticInfo semanticInfo;

			++index;
			for (int c = 0; c < 3; ++c) {

				semantic = m_xmlParse->ReadChildNodeData("input semantic", buffer, index);
				if (!semantic.empty()) {

					if (semantic == "INPUT") {
						semanticInfo.input = m_xmlParse->ReadChildNodeData("source", buffer, index++);
					}
					else if (semantic == "OUTPUT") {
						semanticInfo.output = m_xmlParse->ReadChildNodeData("source", buffer, index++);
					}
					else if (semantic == "INTERPOLATION") {
						semanticInfo.interpolation = m_xmlParse->ReadChildNodeData("source", buffer, index++);
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

			index = m_xmlParse->FindNodeInBuffer("source id", buffer, index);
			if (index == UINT32_MAX) {
				return;
			}

			std::string id = m_xmlParse->ReadChildNodeData("source id", buffer, index++);

			uint32_t arrayCount = 0;
			if (id == m_semanticData[semanticIndex].input) {

				arrayCount = GetFloatArrayInfo(m_semanticData[semanticIndex].input, buffer, index, semanticIndex);
				index = m_xmlParse->ReadChildFloatArray(buffer, animInfo.time, index, arrayCount);
			}
			else if (id == m_semanticData[semanticIndex].output) {

				arrayCount = GetFloatArrayInfo(m_semanticData[semanticIndex].output, buffer, index, semanticIndex);
				index = m_xmlParse->ReadChildFloatArray(buffer, animInfo.transform, index, arrayCount);
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

			index = m_xmlParse->FindNodeInBuffer("source id", buffer, index);
			if (index == UINT32_MAX) {
				return;
			}

			std::string id = m_xmlParse->ReadChildNodeData("source id", buffer, index++);

			uint32_t arrayCount = 0;
			if (id == m_semanticData[semanticIndex].input) {

				arrayCount = GetFloatArrayInfo(m_semanticData[semanticIndex].input, buffer, index, semanticIndex);
				index = m_xmlParse->ReadChildFloatArray(buffer, animInfo.time, index, arrayCount);
			}
			else if (id == m_semanticData[semanticIndex].output) {

				arrayCount = GetFloatArrayInfo(m_semanticData[semanticIndex].output, buffer, index, semanticIndex);
				index = m_xmlParse->ReadChildFloat4x4Array(buffer, animInfo.transform, index, arrayCount);
			}
		}

		m_libraryAnimation4x4Data.push_back(animInfo);
		++semanticIndex;
	}
}

uint32_t SimpleCollada::GetFloatArrayInfo(std::string id, XMLPbuffer& buffer, uint32_t index, uint32_t semanticIndex)
{
	// start by ensuring that this is a float array
	std::string arrayId = m_xmlParse->ReadChildNodeData("float_array id", buffer, index);
	if (arrayId != id + "-array") {
		return 0;
	}

	uint32_t arrayCount = m_xmlParse->ReadChildNodeDataInt("count", buffer, index);
	return arrayCount;
}

void SimpleCollada::ImportGeometryData()
{
	XMLPbuffer buffer;

	// begin by reading geomtry data into a buffer
	buffer = m_xmlParse->ReadNodeIntoBuffer("library_geometries");
	if (m_xmlParse->ReportErrors() == ErrorFlags::XMLP_INCORRECT_FILE_FORMAT) {

		return;
	}

	std::string semantic;
	uint32_t index = 0;

	// Get vertex information before importing data
	index = m_xmlParse->FindNodeInBuffer("/vertices", buffer, index++);
	m_geometryInfo.indicesCount = m_xmlParse->ReadChildNodeDataInt("count", buffer, index++);

	// TODO - allow for multiple meshes
	for (int c = 0; c < 3; ++c) {

		semantic = m_xmlParse->ReadChildNodeData("input semantic", buffer, index);
		if (!semantic.empty()) {

			if (semantic == "VERTEX") {
				m_geometryInfo.vertex.id = m_xmlParse->ReadChildNodeData("source", buffer, index);
				m_geometryInfo.vertex.offset = m_xmlParse->ReadChildNodeDataInt("offset", buffer, index++);
			}
			else if (semantic == "NORMAL") {
				m_geometryInfo.normal.id = m_xmlParse->ReadChildNodeData("source", buffer, index);
				m_geometryInfo.normal.offset = m_xmlParse->ReadChildNodeDataInt("offset", buffer, index++);
			}
			else if (semantic == "TEXCOORD") {
				m_geometryInfo.texcoord.id = m_xmlParse->ReadChildNodeData("source", buffer, index);
				m_geometryInfo.texcoord.offset = m_xmlParse->ReadChildNodeDataInt("offset", buffer, index++);
			}
		}
	}

	index = 0;
	uint32_t count = 0;
	// Read data into buffers - TODO : check counts and stride values of each data set
	index = m_xmlParse->FindNodeInBuffer("mesh", buffer, index);
	index += 2;		// array is located here

	for (int c = 0; c < 3; ++c) {

		std::string sourceId = m_xmlParse->ReadChildNodeData("float_array id", buffer, index);
		if (sourceId == m_geometryInfo.vertex.id) {

			count = m_xmlParse->ReadChildNodeDataInt("count", buffer, index);
			index = m_xmlParse->ReadChildFloatVec3Array(buffer, m_geomtryData.position, index, count);
		}	
		else if (sourceId == m_geometryInfo.normal.id) {

			count = m_xmlParse->ReadChildNodeDataInt("count", buffer, index);
			index = m_xmlParse->ReadChildFloatVec3Array(buffer, m_geomtryData.normal, index, count);
		}	
		else if (sourceId == m_geometryInfo.texcoord.id) {

			count = m_xmlParse->ReadChildNodeDataInt("count", buffer, index);
			index = m_xmlParse->ReadChildFloatVec3Array(buffer, m_geomtryData.texcoord, index, count);
		}
	}
}
