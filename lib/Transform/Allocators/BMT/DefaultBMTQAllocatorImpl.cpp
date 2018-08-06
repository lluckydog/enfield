#include "enfield/Transform/Allocators/BMT/DefaultBMTQAllocatorImpl.h"

using namespace efd;
using namespace bmt;

#include <queue>

// --------------------- SeqNCandidateIterator ------------------------
Node::Ref SeqNCandidateIterator::nextImpl() {
    Node::Ref node = nullptr;

    if (isFirst) {
        isFirst = false;
        mIt = mMod->stmt_begin();
    }

    if (hasNext()) {
        node = mIt->get();
        ++mIt;
    }

    return node;
}

bool SeqNCandidateIterator::hasNextImpl() {
    return mIt != mMod->stmt_end();
}

SeqNCandidateIterator::uRef SeqNCandidateIterator::Create() {
    return uRef(new SeqNCandidateIterator());
}

// --------------------- FirstCandidateSelector ------------------------
CandidateVector FirstCandidateSelector::select(uint32_t maxCandidates,
                                               const CandidateVector& candidates) {
    uint32_t selectedSize = std::min((uint32_t) candidates.size(), maxCandidates);
    CandidateVector selected(candidates.begin(), candidates.begin() + selectedSize);
    return selected;
}

FirstCandidateSelector::uRef FirstCandidateSelector::Create() {
    return uRef(new FirstCandidateSelector());
}

// --------------------- GeoDistanceSwapCEstimator ------------------------
Vector GeoDistanceSwapCEstimator::distanceFrom(Graph::Ref g, uint32_t u) {
    uint32_t size = g->size();

    Vector distance(size, _undef);
    std::queue<uint32_t> q;
    std::vector<bool> visited(size, false);

    q.push(u);
    visited[u] = true;
    distance[u] = 0;

    while (!q.empty()) {
        uint32_t u = q.front();
        q.pop();

        for (uint32_t v : g->adj(u)) {
            if (!visited[v]) {
                visited[v] = true;
                distance[v] = distance[u] + 1;
                q.push(v);
            }
        }
    }

    return distance;
}

void GeoDistanceSwapCEstimator::preprocess() {
    uint32_t size = mG->size();
    mDist.assign(size, Vector());
    for (uint32_t i = 0; i < size; ++i) {
        mDist[i] = distanceFrom(mG, i);
    }
}

uint32_t GeoDistanceSwapCEstimator::estimateImpl(const Mapping& fromM,
                                                 const Mapping& toM) {
    uint32_t totalDistance = 0;

    for (uint32_t i = 0, e = fromM.size(); i < e; ++i) {
        if (fromM[i] != _undef) {
            totalDistance += mDist[fromM[i]][toM[i]];
        }
    }

    return totalDistance;
}

GeoDistanceSwapCEstimator::uRef GeoDistanceSwapCEstimator::Create() {
    return uRef(new GeoDistanceSwapCEstimator());
}

// --------------------- GeoNearestLQPProcessor ------------------------
uint32_t GeoNearestLQPProcessor::getNearest(const Graph::Ref g, uint32_t u, const Assign& assign) {
    std::vector<bool> visited(mPQubits, false);
    std::queue<uint32_t> q;
    q.push(u);
    visited[u] = true;

    while (!q.empty()) {
        uint32_t v = q.front();
        q.pop();

        if (assign[v] == _undef) return v;

        for (uint32_t w : g->adj(v))
            if (!visited[w]) {
                visited[w] = true;
                q.push(w);
            }
    }

    // There is no way we can not find anyone!!
    ERR << "Can't find any vertice connected to v:" << u << "." << std::endl;
    ExitWith(ExitCode::EXIT_unreachable);
}

void GeoNearestLQPProcessor::process(const Graph::Ref g, Mapping& fromM, Mapping& toM) {
    mPQubits = g->size();
    mVQubits = fromM.size();

    auto fromA = InvertMapping(mPQubits, fromM, false);
    auto toA = InvertMapping(mPQubits, toM, false);

    for (uint32_t i = 0; i < mVQubits; ++i) {
        if (toM[i] == _undef && fromM[i] != _undef) {
            if (toA[fromM[i]] == _undef) {
                toM[i] = fromM[i];
            } else {
                toM[i] = getNearest(g, fromM[i], toA);
            }

            toA[toM[i]] = i;
        }
    }
}

GeoNearestLQPProcessor::uRef GeoNearestLQPProcessor::Create() {
    return uRef(new GeoNearestLQPProcessor());
}

// --------------------- BestMSSelector ------------------------
Vector BestMSSelector::select(const TIMatrix& mem) {
    Vector selected;

    uint32_t bestCost = _undef;
    uint32_t bestIdx = _undef;
    uint32_t lastLayer = mem.size() - 1;

    for (uint32_t i = 0, e = mem[lastLayer].size(); i < e; ++i) {
        auto info = mem[lastLayer][i];
        if (info.mappingCost + info.swapEstimatedCost < bestCost) {
            bestCost = info.mappingCost + info.swapEstimatedCost;
            bestIdx = i;
        }
    }

    return { bestIdx };
}

BestMSSelector::uRef BestMSSelector::Create() {
    return uRef(new BestMSSelector());
}
