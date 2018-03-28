#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "glm.hpp"

enum class ErrorFlags
{
	XMLP_ERROR_UNABLE_TO_OPEN_FILE,
	XMLP_ERROR_UNABLE_TO_FIND_NODE,
	XMLP_INCORRECT_FILE_FORMAT,
	XMLP_NO_DATA_IN_SPECIFIED_NODE,
	XMLP_INCORRECT_NODE_FORMAT,
	XMLP_NO_ERROR
};

using XMLPbuffer = std::vector<std::string>;

class XMLparse
{
public:

	XMLparse();
	~XMLparse();

	void LoadXMLDocument(std::string filename);
	void FindNode(std::string node);
	XMLPbuffer ReadNodeIntoBuffer(std::string node);
	uint32_t FindNodeInBuffer(std::string node, XMLPbuffer& buffer, uint32_t index);
	std::string ReadChildNodeData(std::string nodeName, XMLPbuffer& buffer, uint32_t index);
	int ReadChildNodeDataInt(std::string nodeName, XMLPbuffer& buffer, uint32_t index);
	uint32_t ReadChildFloatArray(XMLPbuffer& buffer, std::vector<float>& dstBuffer, uint32_t index, uint32_t arrayCount);
	uint32_t ReadChildFloatVec3Array(XMLPbuffer& buffer, std::vector<glm::vec3>& dstBuffer, uint32_t index, uint32_t arrayCount);
	uint32_t ReadChildFloat4x4Array(XMLPbuffer& buffer, std::vector<glm::mat4>& dstBuffer, uint32_t index, uint32_t arrayCount);
	

	// helper functions
	ErrorFlags ReportErrors() { return m_errorFlags; }

private:

	std::fstream m_file;

	// error flags that can be accessed by the user
	ErrorFlags m_errorFlags;
};

