#include "XMLparse.h"
#include <sstream>

XMLparse::XMLparse() :
	m_errorFlags(ErrorFlags::XMLP_NO_ERROR)
{
}


XMLparse::~XMLparse()
{
}

void XMLparse::LoadXMLDocument(std::string filename)
{

	m_file.open(filename, std::ios_base::in);
	if (!m_file.is_open()) {
		m_errorFlags = ErrorFlags::XMLP_ERROR_UNABLE_TO_OPEN_FILE;
		return;
	}

}

void XMLparse::FindElement(std::string node)
{
	node = '<' + node + '>';		// convert to xml type 
	
	std::string str;
	while (getline(m_file, str)) {

		std::stringstream ss(str);

		std::string line;
		ss >> line;
		if (line == node) {

			return;
		}
	}

	m_errorFlags = ErrorFlags::XMLP_ERROR_UNABLE_TO_FIND_NODE;
}

XMLPbuffer XMLparse::ReadTreeIntoBuffer(std::string node)
{
	m_errorFlags = ErrorFlags::XMLP_NO_ERROR;
	m_file.clear();
	m_file.seekg(0, std::ios::beg);

	std::string startNode = '<' + node + '>';
	std::string endNode("</" + node + '>');

	XMLPbuffer buffer;

	std::string str;
	while (getline(m_file, str)) {

		size_t pos = str.find(startNode);
		if (pos != std::string::npos) {

			while (getline(m_file, str)) {			// node found, so start nested loop to find the end

				pos = str.find(endNode);
				if (pos != std::string::npos) {
					return buffer;					// found the end of this node
				}
				else {
					buffer.push_back(str);			// else, add data to the buffer
				}
			}
		}
	}

	// something went wrong whilst reading the file
	m_errorFlags = ErrorFlags::XMLP_INCORRECT_FILE_FORMAT;

	return buffer;
}

uint32_t XMLparse::FindElementInBuffer(std::string node, XMLPbuffer& buffer, uint32_t index)
{
	size_t pos = 0;
	std::string line;

	node = '<' + node;

	for (int c = index; c < buffer.size(); ++c) {

		line = buffer[c];
		pos = line.find(node);
		if (pos != std::string::npos) {

			return c;
		}
		
	}

	return UINT32_MAX;
}

std::string XMLparse::ReadElementDataString(std::string node, XMLPbuffer& buffer, uint32_t index)
{
	std::string str = buffer[index];
	
	if (str.empty()) {
		m_errorFlags = ErrorFlags::XMLP_NO_DATA_IN_SPECIFIED_NODE;
		return "";
	}

	size_t pos = str.find(node);
	if (pos == std::string::npos) {
		m_errorFlags = ErrorFlags::XMLP_INCORRECT_NODE_FORMAT;
		return "";
	}

	str = str.substr(pos + node.size(), str.size());

	// remove the colons around and including the required data
	pos = str.find_first_of('"');
	if (pos != std::string::npos) {

		str = str.substr(pos + 1, str.size());
		pos = str.find_first_of('"');
		if (pos != std::string::npos) {

			std::string outputStr = str.substr(0, pos);
			if (!outputStr.empty()) {
				
				if (outputStr[0] == '#') {				// certain packages place a hash symbol before the string which we dont need
					outputStr.erase(0, 1);
				}
				return outputStr;
			}
		}
	}
	m_errorFlags = ErrorFlags::XMLP_NO_DATA_IN_SPECIFIED_NODE;
	return "";
}

int XMLparse::ReadElementDataInt(std::string nodeName, XMLPbuffer& buffer, uint32_t index)
{
	std::string str = ReadElementDataString(nodeName, buffer, index);
	if (str.empty()) {
		return 0;
	}

	int value = std::stoi(str);
	return value;
}

float XMLparse::ReadElementDataFloat(std::string nodeName, XMLPbuffer& buffer, uint32_t index)
{
	std::string startNode = '<' + nodeName + '>';

	std::string str = buffer[index];

	if (str.empty()) {
		m_errorFlags = ErrorFlags::XMLP_NO_DATA_IN_SPECIFIED_NODE;
		return 0.0f;
	}

	size_t pos = str.find(startNode);
	if (pos == std::string::npos) {
		m_errorFlags = ErrorFlags::XMLP_INCORRECT_NODE_FORMAT;
		return 0.0f;
	}

	pos = str.find_first_of('>');
	str = str.substr(pos + 1, str.size());

	std::stringstream ss(str);
	float num;
	ss >> num;

	return num;
}

glm::mat4 XMLparse::ReadElementDataMatrix(std::string nodeName, XMLPbuffer& buffer, uint32_t index)
{
	std::string startNode = '<' + nodeName;

	std::string str = buffer[index];

	if (str.empty()) {
		m_errorFlags = ErrorFlags::XMLP_NO_DATA_IN_SPECIFIED_NODE;
		return glm::mat4(0.0f);
	}

	size_t pos = str.find(startNode);
	if (pos == std::string::npos) {
		m_errorFlags = ErrorFlags::XMLP_INCORRECT_NODE_FORMAT;
		return glm::mat4(0.0f);
	}

	pos = str.find_first_of('>');
	str = str.substr(pos + 1, str.size());

	glm::mat4 mat;
	std::stringstream ss(str);

	for (int x = 0; x < 3; ++x) {

		for (int y = 0; y < 3; ++y) {

			ss >> mat[x][y];
		}
	}

	return mat;
}

uint32_t XMLparse::ReadElementArrayVec3(XMLPbuffer& buffer, std::vector<glm::vec3>& dstBuffer, uint32_t index, uint32_t arrayCount)
{
	uint32_t arrayIndex = index;

	// start by removing array data from first line
	std::string str = buffer[arrayIndex];

	size_t pos = str.find_first_of(">");
	if (pos == std::string::npos) {
		m_errorFlags = ErrorFlags::XMLP_INCORRECT_NODE_FORMAT;
		return index;
	}
	str = str.substr(pos + 1, str.size());

	uint32_t count = 0;

	while (arrayIndex < buffer.size()) {

		glm::vec3 vec;
		std::stringstream ss(str);

		for (int x = 0; x < 3; ++x) {

			ss >> vec[x];
			if (ss.eof()) {

				++arrayCount;
				str = buffer[arrayCount];
				assert(arrayCount < buffer.size());
			}
		}
		dstBuffer.push_back(vec);
		++count;
		if (count == arrayCount / 3) {				// stride of 3
			return arrayIndex + 1;					// point to the next line in the file after the array
		}
	}

	// if we arrived here, the there's a problem with the file formatting
	m_errorFlags = ErrorFlags::XMLP_INCORRECT_FILE_FORMAT;
	return arrayIndex;
}

uint32_t XMLparse::ReadElementArrayMatrix(XMLPbuffer& buffer, std::vector<glm::mat4>& dstBuffer, uint32_t index, uint32_t arrayCount)
{
	uint32_t arrayIndex = index;

	// start by removing array data from first line
	std::string str = buffer[arrayIndex];

	size_t pos = str.find_first_of(">");
	if (pos == std::string::npos) {
		m_errorFlags = ErrorFlags::XMLP_INCORRECT_NODE_FORMAT;
		return index;
	}
	str = str.substr(pos + 1, str.size());

	uint32_t count = 0;

	while (arrayIndex < buffer.size()) {
		
		glm::mat4 mat;
		std::stringstream ss(str);

		for (int x = 0; x < 3; ++x) {

			for (int y = 0; y < 3; ++y) {

				ss >> mat[x][y];
				if(ss.eof()) {

					++arrayCount;
					str = buffer[arrayCount];
					assert(arrayCount < buffer.size());
				}
			}
		}
		dstBuffer.push_back(mat);
		++count;
		if (count == arrayCount / 16) {				// stride 4x4  = 16
			return arrayIndex + 1;					// point to the next line in the file after the array
		}
	}

	// if we arrived here, the there's a problem with the file formatting
	m_errorFlags = ErrorFlags::XMLP_INCORRECT_FILE_FORMAT;
	return arrayIndex;
}

bool XMLparse::CheckElement(std::string id, XMLPbuffer& buffer, uint32_t index)
{
	id = '<' + id;
	std::string str = buffer[index];

	size_t pos = str.find(id);
	if (pos != std::string::npos) {

		return true;
	}

	return false;
}
