#include "gtest/gtest.h"

#include "bout/constants.hxx"
#include "bout/mesh.hxx"
#include "bout/region.hxx"
#include "boutexception.hxx"
#include "output.hxx"
#include "test_extras.hxx"
#include "unused.hxx"

#include <cmath>
#include <set>
#include <vector>

/// Global mesh
extern Mesh *mesh;

/// Test fixture to make sure the global mesh is our fake one
class RegionTest : public ::testing::Test {
protected:
  static void SetUpTestCase() {
    // Delete any existing mesh
    if (mesh != nullptr) {
      delete mesh;
      mesh = nullptr;
    }
    mesh = new FakeMesh(nx, ny, nz);
    mesh->createDefaultRegions();
  }

  static void TearDownTestCase() {
    delete mesh;
    mesh = nullptr;
  }

public:
  static const int nx;
  static const int ny;
  static const int nz;
};

const int RegionTest::nx = 3;
const int RegionTest::ny = 5;
const int RegionTest::nz = 7;

TEST_F(RegionTest, maxBlockSize) { EXPECT_TRUE(MAXREGIONBLOCKSIZE > 0); }

TEST_F(RegionTest, regionFromRange) {
  Region<Ind3D> region(0, mesh->LocalNx - 1, 0, mesh->LocalNy - 1, 0, mesh->LocalNz - 1,
                       mesh->LocalNy, mesh->LocalNz);

  int nmesh = RegionTest::nx * RegionTest::ny * RegionTest::nz;
  auto regionIndices = region.getIndices();

  // Check index vector is expected length
  EXPECT_EQ(regionIndices.size(), nmesh);

  for (int i = 0; i < nmesh; i++) {
    EXPECT_EQ(regionIndices[i].ind, i);
  }
}

TEST_F(RegionTest, regionFromIndices) {
  std::vector<std::pair<int, int>> blocksIn = {
      {0, 3}, {5, 7}, {9, 10}, {12, 12}, {14, 20}};
  std::vector<Ind3D> indicesIn;
  int maxContiguousSizeUsed = 0;
  for (auto &block : blocksIn) {
    int currBlockSize = 1 + block.second - block.first;
    maxContiguousSizeUsed =
        currBlockSize > maxContiguousSizeUsed ? currBlockSize : maxContiguousSizeUsed;
    for (int i = block.first; i <= block.second; i++) {
      indicesIn.push_back(i);
    }
  }

  // If our region block size is too small then this test won't work
  if (maxContiguousSizeUsed > MAXREGIONBLOCKSIZE) {
    return;
  }

  Region<Ind3D> region(indicesIn);

  auto regionIndices = region.getIndices();

  // Check index vector is expected length
  EXPECT_EQ(regionIndices.size(), indicesIn.size());

  for (unsigned int i = 0; i < indicesIn.size(); i++) {
    EXPECT_EQ(regionIndices[i], indicesIn[i]);
  }

  auto regionBlocks = region.getBlocks();

  // Check expected number of blocks
  EXPECT_EQ(regionBlocks.size(), blocksIn.size());

  for (unsigned int i = 0; i < blocksIn.size(); i++) {
    EXPECT_EQ(regionBlocks[i].first.ind, blocksIn[i].first);
    // The actual block second is exclusive, blocksIn.second is inclusive
    EXPECT_EQ(regionBlocks[i].second.ind - 1, blocksIn[i].second);
  }
}

TEST_F(RegionTest, numberOfBlocks) {
  Region<Ind3D> region(0, mesh->LocalNx - 1, 0, mesh->LocalNy - 1, 0, mesh->LocalNz - 1,
                       mesh->LocalNy, mesh->LocalNz);

  auto blocks = region.getBlocks();
  int nmesh = RegionTest::nx * RegionTest::ny * RegionTest::nz;
  int nblocks = blocks.size();

  // Calculate expected number of blocks, assuming region is entirely
  // contiguous, as is the case here.
  int nexpected = nmesh / MAXREGIONBLOCKSIZE; // Integer division required
  // Above is only correct when MAXREGIONBLOCKSIZE is a factor of nmesh,
  // so check if there needs to be an extra block for remainder
  if ((1.0 * nmesh) / MAXREGIONBLOCKSIZE > 0) {
    nexpected++;
  }

  EXPECT_EQ(nexpected, nblocks);
}

TEST_F(RegionTest, contiguousBlockSize) {
  Region<Ind3D> region(0, mesh->LocalNx - 1, 0, mesh->LocalNy - 1, 0, mesh->LocalNz - 1,
                       mesh->LocalNy, mesh->LocalNz);

  const int nmesh = RegionTest::nx * RegionTest::ny * RegionTest::nz;

  auto blocks = region.getBlocks();
  const int nblocks = blocks.size();
  auto firstBlock = blocks[0];
  auto lastBlock = blocks[nblocks - 1];

  // Calculate expected size of blocks, assuming region is entirely
  // contiguous, as is the case here.
  const int expectedFirstBlockSize =
      nmesh >= MAXREGIONBLOCKSIZE ? MAXREGIONBLOCKSIZE : nmesh;
  EXPECT_EQ(firstBlock.second.ind - firstBlock.first.ind, expectedFirstBlockSize);
  const int expectedLastBlockSize = nmesh % MAXREGIONBLOCKSIZE;
  if (expectedLastBlockSize != 0) {
    EXPECT_EQ(lastBlock.second.ind - lastBlock.first.ind, expectedLastBlockSize);
  } else {
    // If no remainder then expect same block size as in first block
    EXPECT_EQ(lastBlock.second.ind - lastBlock.first.ind, expectedFirstBlockSize);
  }
}

TEST_F(RegionTest, defaultRegions) {
  const int nmesh = RegionTest::nx * RegionTest::ny * RegionTest::nz;
  EXPECT_EQ(mesh->getRegion("RGN_ALL").getIndices().size(), nmesh);

  const int nnobndry =
      RegionTest::nz * (mesh->yend - mesh->ystart + 1) * (mesh->xend - mesh->xstart + 1);
  EXPECT_EQ(mesh->getRegion("RGN_NOBNDRY").getIndices().size(), nnobndry);

  const int nnox = RegionTest::nz * RegionTest::ny * (mesh->xend - mesh->xstart + 1);
  EXPECT_EQ(mesh->getRegion("RGN_NOX").getIndices().size(), nnox);

  const int nnoy = RegionTest::nz * RegionTest::nx * (mesh->yend - mesh->ystart + 1);
  EXPECT_EQ(mesh->getRegion("RGN_NOY").getIndices().size(), nnoy);
}

TEST_F(RegionTest, regionLoopAll) {
  auto region = mesh->getRegion3D("RGN_ALL");

  Field3D a = 0.0;
  BLOCK_REGION_LOOP(region, i, a[i] = 1.0;);

  for (const auto &i : a.region(RGN_ALL)) {
    EXPECT_EQ(a[i], 1.0);
  }
}

TEST_F(RegionTest, regionLoopNoBndry) {
  auto region = mesh->getRegion3D("RGN_NOBNDRY");

  Field3D a = 0.0;
  BLOCK_REGION_LOOP(region, i, a[i] = 1.0;);

  const int nmesh = RegionTest::nx * RegionTest::ny * RegionTest::nz;
  const int ninner =
      (mesh->LocalNz * (1 + mesh->xend - mesh->xstart) * (1 + mesh->yend - mesh->ystart));
  int numExpectNotMatching = nmesh - ninner;

  int numNotMatching = 0;
  int numMatching = 0;
  for (const auto &i : a.region(RGN_ALL)) {
    if (a[i] != 1.0) {
      numNotMatching++;
    } else {
      numMatching++;
    }
  }
  EXPECT_EQ(numNotMatching, numExpectNotMatching);
  EXPECT_EQ(numMatching, ninner);
}

TEST_F(RegionTest, regionLoopAllSerial) {
  auto region = mesh->getRegion3D("RGN_ALL");

  Field3D a = 0.0;
  BLOCK_REGION_LOOP_SERIAL(region, i, a[i] = 1.0;);

  for (const auto &i : a.region(RGN_ALL)) {
    EXPECT_EQ(a[i], 1.0);
  }
}

TEST_F(RegionTest, regionLoopNoBndrySerial) {
  auto region = mesh->getRegion3D("RGN_NOBNDRY");

  Field3D a = 0.0;
  BLOCK_REGION_LOOP_SERIAL(region, i, a[i] = 1.0;);

  const int nmesh = RegionTest::nx * RegionTest::ny * RegionTest::nz;
  const int ninner =
      (mesh->LocalNz * (1 + mesh->xend - mesh->xstart) * (1 + mesh->yend - mesh->ystart));
  int numExpectNotMatching = nmesh - ninner;

  int numNotMatching = 0;
  int numMatching = 0;
  for (const auto &i : a.region(RGN_ALL)) {
    if (a[i] != 1.0) {
      numNotMatching++;
    } else {
      numMatching++;
    }
  }
  EXPECT_EQ(numNotMatching, numExpectNotMatching);
  EXPECT_EQ(numMatching, ninner);
}

TEST_F(RegionTest, regionAsSorted) {
  // Contiguous blocks to insert
  std::vector<std::pair<int, int>> blocksIn = {
      {0, 3}, {5, 7}, {9, 10}, {12, 12}, {14, 20}};
  Region<Ind3D>::RegionIndices indicesIn;

  for (auto &block : blocksIn) {
    for (int i = block.first; i <= block.second; i++) {
      indicesIn.push_back(i);
    }
  }

  // This is the sorted region and indices
  Region<Ind3D> regionSortedIn(indicesIn);
  Region<Ind3D>::RegionIndices regionIndicesSortedIn = regionSortedIn.getIndices();

  // Now shuffle the order and create a new region
  std::random_shuffle(std::begin(indicesIn), std::end(indicesIn));
  Region<Ind3D> regionShuffledIn(indicesIn);
  Region<Ind3D>::RegionIndices regionIndicesShuffledIn = regionShuffledIn.getIndices();
  // Should we check the shuffle has actually changed the index order?
  // If it hasn't the test isn't really testing sorted

  // Check index vectors haven't changed length
  EXPECT_EQ(regionIndicesShuffledIn.size(), regionIndicesSortedIn.size());

  // Now get a sorted verion of the region
  Region<Ind3D> regionAsSorted = regionShuffledIn.asSorted();
  Region<Ind3D>::RegionIndices regionIndicesAsSorted = regionAsSorted.getIndices();

  for (unsigned int i = 0; i < regionIndicesSortedIn.size(); i++) {
    EXPECT_EQ(regionIndicesAsSorted[i], regionIndicesSortedIn[i]);
  }
}

TEST_F(RegionTest, regionAsUnique) {
  // Values to insert
  std::vector<int> rawIndicesIn1 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                                    8, 7, 6, 5, 4, 3, 2, 1, 0};

  Region<Ind3D>::RegionIndices indicesIn1;
  for (auto i : rawIndicesIn1) {
    indicesIn1.push_back(i);
  }

  // This is the sorted region and indices
  Region<Ind3D> regionIn1(indicesIn1);
  Region<Ind3D>::RegionIndices regionIndicesIn1 = regionIn1.getIndices();

  // Now get a unique version of the region
  Region<Ind3D> regionUnique1 = regionIn1.asUnique();
  Region<Ind3D>::RegionIndices regionIndicesUnique1 = regionUnique1.getIndices();

  EXPECT_EQ(regionIndicesUnique1.size(), 10);
  for (unsigned int i = 0; i < regionIndicesUnique1.size(); i++) {
    EXPECT_EQ(regionIndicesUnique1[i], i);
  }

  std::vector<int> rawIndicesIn2 = {0, 0, 0, 2, 2, 2, 5, 4, 4, 5, 10, 12, 11, 9};

  Region<Ind3D>::RegionIndices indicesIn2;
  for (auto i : rawIndicesIn2) {
    indicesIn2.push_back(i);
  }

  // This is the sorted region and indices
  Region<Ind3D> regionIn2(indicesIn2);
  Region<Ind3D>::RegionIndices regionIndicesIn2 = regionIn2.getIndices();

  // Now get a unique version of the region
  Region<Ind3D> regionUnique2 = regionIn2.asUnique();
  Region<Ind3D>::RegionIndices regionIndicesUnique2 = regionUnique2.getIndices();

  EXPECT_EQ(regionIndicesUnique2.size(), 8);

  std::vector<int> rawIndicesIn3 = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

  Region<Ind3D>::RegionIndices indicesIn3;
  for (auto i : rawIndicesIn3) {
    indicesIn3.push_back(i);
  }

  // This is the sorted region and indices
  Region<Ind3D> regionIn3(indicesIn3);
  Region<Ind3D>::RegionIndices regionIndicesIn3 = regionIn3.getIndices();

  // Now get a unique version of the region
  Region<Ind3D> regionUnique3 = regionIn3.asUnique();
  Region<Ind3D>::RegionIndices regionIndicesUnique3 = regionUnique3.getIndices();

  EXPECT_EQ(regionIndicesUnique3.size(), 10);
  for (unsigned int i = 0; i < regionIndicesUnique3.size(); i++) {
    EXPECT_EQ(regionIndicesUnique3[i], i);
  }
}

TEST_F(RegionTest, regionSetIndices) {
  // Values to insert
  std::vector<int> rawIndicesIn1 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  std::vector<int> rawIndicesIn2 = {10, 11, 12};

  Region<Ind3D>::RegionIndices indicesIn1;
  for (auto i : rawIndicesIn1) {
    indicesIn1.push_back(i);
  }

  Region<Ind3D>::RegionIndices indicesIn2;
  for (auto i : rawIndicesIn2) {
    indicesIn2.push_back(i);
  }

  // This is the sorted region and indices
  Region<Ind3D> regionIn1(indicesIn1);
  Region<Ind3D>::RegionIndices regionIndicesIn1 = regionIn1.getIndices();
  EXPECT_EQ(regionIndicesIn1.size(), indicesIn1.size());

  for (unsigned int i = 0; i < regionIndicesIn1.size(); i++) {
    EXPECT_EQ(regionIndicesIn1[i], indicesIn1[i]);
  }

  regionIn1.setIndices(indicesIn2);
  Region<Ind3D>::RegionIndices regionIndicesIn2 = regionIn1.getIndices();
  EXPECT_EQ(regionIndicesIn2.size(), indicesIn2.size());

  for (unsigned int i = 0; i < regionIndicesIn2.size(); i++) {
    EXPECT_EQ(regionIndicesIn2[i], indicesIn2[i]);
  }
}
