#ifndef __EFD_QBIT_ALLOCATOR_H__
#define __EFD_QBIT_ALLOCATOR_H__

#include "enfield/Support/Graph.h"
#include "enfield/Support/SwapFinder.h"
#include "enfield/Transform/DependencyBuilderPass.h"

namespace efd {
    /// \brief Base abstract class that allocates the qbits used in the program to
    /// the qbits that are in the physical architecture.
    class QbitAllocator {
        public:
            typedef std::vector<std::string> Mapping;

            typedef efd::SwapFinder::RestrictionVector RestrictionVector;
            typedef efd::SwapFinder::Rest Rest;
            typedef efd::SwapFinder::SwapVector SwapVector;
            typedef efd::SwapFinder::Swap Swap;

            typedef efd::DependencyBuilderPass::DepsSet DepsSet;
            typedef DepsSet::iterator Iterator;

        private:
            DependencyBuilderPass* mDepPass;
            DepsSet mDepSet;
            Mapping mMapping;
            bool mRun;

            /// \brief Updates the mDepSet attribute. Generally it is done after
            /// running the DependencyBuilderPass.
            void updateDepSet();

            /// \brief Constructs a node from an unsigned id.
            NodeRef getIdNodeFromString(std::string s);

        protected:
            QModule* mMod;
            Graph* mPhysGraph;
            SwapFinder* mSFind;

            QbitAllocator(QModule* qmod, Graph* pGraph, SwapFinder* sFind,
                    DependencyBuilderPass* depPass);

            /// \brief Inlines the gate call that generates the dependencies that are
            /// referenced by \p it. If the node is not an NDQOpGeneric, it does nothing.
            Iterator inlineDep(Iterator it);

            /// \brief Inserts a swap between u and v. (note that these indexes must be
            /// the indexes of the program's qbit)
            virtual void insertSwapBefore(Dependencies& deps, unsigned u, unsigned v);

        public:
            /// \brief Runs the allocator;
            void run();
            /// \brief Returns the final mapping.
            Mapping getMapping();

            /// \brief Generates the final mapping of the program, inserting the swaps where
            /// needed.
            virtual Mapping solveDependencies(DepsSet& deps) = 0;
    };
}

#endif
