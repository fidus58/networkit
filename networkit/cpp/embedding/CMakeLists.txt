set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

networkit_add_module(embedding
    AliasSampler.cpp
    BiasedRandomWalk.cpp
    LearnEmbedding.cpp
    Node2Vec.cpp
    )

networkit_module_link_modules(embedding
        auxiliary base graph)

add_subdirectory(test)

