/*
 * EdgeListReader.cpp
 *
 *  Created on: 18.06.2013
 *      Author: cls
 */

#include <fstream>
#include <sstream>

#include <networkit/auxiliary/Enforce.hpp>
#include <networkit/auxiliary/Log.hpp>
#include <networkit/io/EdgeListReader.hpp>

namespace NetworKit {

EdgeListReader::EdgeListReader(const char separator, const node firstNode, const std::string commentPrefix, const bool continuous, const bool directed) :
    separator(separator), commentPrefix(commentPrefix), firstNode(firstNode), continuous(continuous), mapNodeIds(), directed(directed) {}

Graph EdgeListReader::read(const std::string& path) {
    this->mapNodeIds.clear();

    if (this->continuous) {
        DEBUG("read graph with continuous ids");
        return readContinuous(path);
    } else {
        DEBUG("read graph with NON continuous ids");
        return readNonContinuous(path);
    }
}

const std::map<std::string,node> &EdgeListReader::getNodeMap() const {
    if (this->continuous) throw std::runtime_error("Input files are assumed to have continuous node ids, therefore no node mapping has been created.");
    return this->mapNodeIds;
}

void EdgeListReader::malformedLineError(count lineNumber, const std::string &line) {
    std::stringstream message;
    message << "malformed line ";
    message << lineNumber << ": ";
    message << line;
    throw std::runtime_error(message.str());
}

Graph EdgeListReader::readContinuous(const std::string& path) {
    std::ifstream file(path);
    Aux::enforceOpened(file);
    std::string line; // the current line

    // read file once to get to the last line and figure out the number of nodes
    // unfortunately there is an empty line at the ending of the file, so we need to get the line before that

    node maxNode = 0;
    bool weighted = false;
    bool checkedWeighted = false;
    bool ignoredParameters = false;

    DEBUG("separator: " , this->separator);
    DEBUG("first node: " , this->firstNode);

    // first find out the maximum node id
    DEBUG("first pass");
    count i = 0;
    while (file.good()) {
        ++i;
        std::getline(file, line);
        TRACE("read line: " , line);
        if (!line.empty()) {
            if (line.back() == '\r') line.pop_back();
            if (line.compare(0, this->commentPrefix.length(), this->commentPrefix) == 0) {
                TRACE("ignoring comment: " , line);
            } else {
                std::vector<std::string> split = Aux::StringTools::split(line, this->separator);
                if (!checkedWeighted) {
                    if (split.size() == 2 || split.size() > 3) {
                        weighted = false;
                        ignoredParameters = split.size() > 3;
                    } else if (split.size() == 3) {
                        INFO("Identified graph as weighted.");
                        weighted = true;
                    }
                    checkedWeighted = true;
                }

                if (split.size() < 2)
                    malformedLineError(i, line);

                TRACE("split into : " , split[0] , " and " , split[1]);
                node u = std::stoul(split[0]);
                if (u > maxNode) {
                    maxNode = u;
                }
                node v = std::stoul(split[1]);
                if (v > maxNode) {
                    maxNode = v;
                }
                if (split.size() > 3)
                    ignoredParameters = true;
            }
        } else {
            DEBUG("line ", i, " is empty.");
        }
    }
    file.close();

    if (ignoredParameters)
        WARN("Parameters after the third position are ignored");

    maxNode = maxNode - this->firstNode + 1;
    DEBUG("max. node id found: " , maxNode);

    Graph G(maxNode, weighted, directed);

    DEBUG("second pass");
    file.open(path);
    // split the line into start and end node. since the edges are sorted, the start node has the highest id of all nodes
    i = 0; // count lines
    while(std::getline(file,line)){
        if (line.empty()) continue;
        if(*line.rbegin() == '\r') line.pop_back();
        ++i;
        if (line.compare(0, this->commentPrefix.length(), this->commentPrefix) != 0) {
            std::vector<std::string> split = Aux::StringTools::split(line, this->separator);

            node u = std::stoul(split[0]) - this->firstNode;
            node v = std::stoul(split[1]) - this->firstNode;
            if (G.hasEdge(u, v))
                continue;
            if (!weighted)
                G.addEdge(u, v);
            else if (split.size() < 3)
                malformedLineError(i, line);
            else
                G.addEdge(u, v, std::stod(split[2]));
        }
    }
    file.close();

    G.shrinkToFit();
    return G;
}


Graph EdgeListReader::readNonContinuous(const std::string& path) {
    std::ifstream file(path);
    Aux::enforceOpened(file);
    DEBUG("file is opened, proceed");
    std::string line; // the current line
    node consecutiveID = 0;

    bool weighted = false;
    bool checkedWeighted = false;
    bool ignoredParameters = false;

    // first find out the maximum node id
    DEBUG("first pass: create node ID mapping");
    count i = 0;
    while (file.good()) {
        ++i;
        std::getline(file, line);
        TRACE("read line: " , line);
        if (!line.empty()) {
            if(line.back() == '\r') line.pop_back();
            if (line.compare(0, this->commentPrefix.length(), this->commentPrefix) == 0) {
                 TRACE("ignoring comment: " , line);
            } else if (line.length() == 0) {
                TRACE("ignoring empty line");
            } else {
                std::vector<std::string> split = Aux::StringTools::split(line, this->separator);
                if (!checkedWeighted) {
                    if (split.size() == 2 || split.size() > 3) {
                        weighted = false;
                        ignoredParameters = split.size() > 3;
                    } else if (split.size() == 3) {
                        INFO("Identified graph as weighted.");
                        weighted = true;
                    }
                    checkedWeighted = true;
                }
                if (split.size() < 2)
                    malformedLineError(i,  line);

                TRACE("split into : " , split[0] , " and " , split[1]);
                if(this->mapNodeIds.emplace(split[0], consecutiveID).second) ++consecutiveID;
                if(this->mapNodeIds.emplace(split[1], consecutiveID).second) ++consecutiveID;
                if (split.size() > 3)
                    ignoredParameters = true;
            }
        } else {
            DEBUG("line ", i, " is empty.");
        }
    }
    file.close();

    if (ignoredParameters)
        WARN("Parameters after the third position are ignored");
    DEBUG("found ",this->mapNodeIds.size()," unique node ids");
    Graph G(this->mapNodeIds.size(), weighted, directed);

    DEBUG("second pass: add edges");
    file.open(path);

    // split the line into start and end node. since the edges are sorted, the start node has the highest id of all nodes
    i = 0; // count lines
    while(std::getline(file,line)){
        if (line.empty()) continue;
        if(*line.rbegin() == '\r') line.pop_back();
        ++i;
        if (line.compare(0, this->commentPrefix.length(), this->commentPrefix) != 0) {
            std::vector<std::string> split = Aux::StringTools::split(line, this->separator);

            node u = this->mapNodeIds.at(split[0]);
            node v = this->mapNodeIds.at(split[1]);
            if (G.hasEdge(u, v))
                continue;
            if (!weighted)
                G.addEdge(u, v);
            else if (split.size() < 3)
                malformedLineError(i, line);
            else
                G.addEdge(u, v, std::stod(split[2]));
        }
    }
    DEBUG("read ",i," lines and added ",G.numberOfEdges()," edges");
    file.close();

    G.shrinkToFit();
    return G;
}

} /* namespace NetworKit */
