#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <queue>
#include <random>
#include <ratio>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

unordered_map<uint64_t, vector<uint64_t> > bookmarks;
unordered_map<uint64_t, vector<uint64_t> > bookmarkReverse;

const uint64_t USER_COUNT = 10000;
const uint64_t AVERAGE_BOOKMARK = 1000;
const uint64_t WORK_COUNT = 100000;

unordered_map<uint64_t, unordered_set<string> > workTags;

/* Generator */

random_device rd;
mt19937_64 gen(rd());

geometric_distribution<> skip_dist(((double) AVERAGE_BOOKMARK)/WORK_COUNT);
uniform_int_distribution<> tag_dist(0, 1);

const vector<string> TAGS = {
  "A",
  "B",
  "C",
  "D",
};

void bookmark(uint64_t user, uint64_t work) {
  bookmarks[user].push_back(work);
  bookmarkReverse[work].push_back(user);
}

uint64_t last_milestone = 0;

void generateDataSet() {

  // Generate tags

  cout<<"Generating tags..."<<endl;

  for(uint64_t i = 0; i < WORK_COUNT; ++i) {
    auto &tagSet = workTags[i];
    for(const auto &tag : TAGS)
      if(tag_dist(gen) == 1)
        tagSet.insert(tag);
  }

  cout<<"Tags generated."<<endl;

  // Generate bookmarks

  cout<<"Generating bookmarks..."<<endl;

  uint64_t current = 0;

  while(current < USER_COUNT * WORK_COUNT) {
    if(current - last_milestone > 100000000) {
      last_milestone = current;
      cout<<"Progress: "<<((double) current) / (USER_COUNT * WORK_COUNT)<<endl;
    }

    bookmark(current / WORK_COUNT, current % WORK_COUNT);
    current+=skip_dist(gen);
  }
}

/* Implementation */

const size_t MAX_BATCH_SIZE = 200;
const size_t RECOMMENDER_LIMIT = 10;
const size_t MAX_SOURCE_COUNT = 100;
const size_t SOURCE_SIMILARITY_LOWERBOUND = 10;

vector<uint64_t> fetchRecommendation(uint64_t user, const vector<string> requiredTags, size_t size) {
  if(bookmarks[user].size() < RECOMMENDER_LIMIT) return {};
  uniform_int_distribution<uint64_t> next_sample(0, bookmarks[user].size() - 1);

  // User map, user ID -> reference count
  unordered_map<uint64_t, uint64_t> userStash;

  for(int i = 0; i < MAX_BATCH_SIZE; ++i) {
    uint64_t sample = next_sample(gen);
    auto &bookmarkerList = bookmarkReverse[sample];

    for(uint64_t bk : bookmarkerList)
      ++userStash[bk];
  }

  // User list, user's reference count as its key
  // Acutally a priority queue should be used here
  // But I'm just too lazy to do that
  vector<pair<uint64_t, uint64_t> >prioritizedUserList(userStash.size());
  size_t listTop = 0;

  for(auto i = userStash.begin(); i != userStash.end(); ++i) {
    prioritizedUserList[listTop] = { i->second, i->first };
    ++listTop;
  }

  // Sort users based on their reference count, greater ones appear near the front
  sort(prioritizedUserList.begin(), prioritizedUserList.end(), greater<>());

  // Last valid recommendation source
  auto lastSource = upper_bound(
      prioritizedUserList.begin(),
      prioritizedUserList.end(),
      make_pair<uint64_t, uint64_t>(SOURCE_SIMILARITY_LOWERBOUND, -1),
      greater<>());

  if(lastSource == prioritizedUserList.end()) // No valid recommendation source
    return {};

  if(lastSource - prioritizedUserList.begin() > MAX_SOURCE_COUNT - 1) {
    // We have valid sources more than MAX_SOURCE_COUNT
    lastSource = prioritizedUserList.begin() + MAX_SOURCE_COUNT - 1;
  }

  // Potential recommended works, work ID -> reference count
  unordered_map<uint64_t, uint64_t> potentialRec;

  for(auto user = prioritizedUserList.begin(); user <= lastSource; ++user)
    for(const auto &work : bookmarks[user->second]) ++potentialRec[work];
    // Mapping for works not previously added are created automatically, with its value set to 0

  // Search result, work's reference count as its key
  // Works with fewer references are in the front
  //   for that we need to extract them
  priority_queue<pair<uint64_t, uint64_t> > results;

  for(const auto &work : potentialRec) {

    // Tags of this work
    const auto &currentTags = workTags[work.first];
    bool flag = true;
    for(const auto &tag : requiredTags)
      if(currentTags.count(tag) == 0) {
        flag = false;
        break;
      }

    if(flag) { // This is a work with required tags
      if(results.size() < size) results.emplace(work.second, work.first);
      else if(work.second > results.top().first) {
        results.emplace(work.second, work.first);
        results.pop();
      }
    }
  }

  // Results we actually returns
  vector<uint64_t> normalizedResults(results.size());

  for(auto i = normalizedResults.rbegin(); i != normalizedResults.rend(); ++i) {
    *i = results.top().second;
    results.pop();
  }

  return normalizedResults;
}

// Testing

uniform_int_distribution<uint64_t> next_user(0, USER_COUNT-1);

const size_t TEST_RESULT_SIZE = 20;
const size_t TEST_BATCH = 10;
const size_t TEST_BATCH_COUNT = 10;

void testOnce() {
  vector<string> requiredTags;
  for(const auto &tag : TAGS)
    if(tag_dist(gen) == 1)
      requiredTags.push_back(tag);

  uint64_t user = next_user(gen);

  auto results = fetchRecommendation(user, requiredTags, TEST_RESULT_SIZE);
  assert(all_of(results.begin(), results.end(), [&requiredTags](uint64_t workId) -> bool {
          for(const auto &tag : requiredTags)
            if(workTags[workId].count(tag) == 0)
              return false;
          return true;
        }));
}

int main() {
  generateDataSet();
  cout<<"---Complete---"<<endl;

  cout<<"---Random Testing---"<<endl;
  cout<<"---At batch of "<<TEST_BATCH<<"---"<<endl<<endl;

  double totalTimeCount = 0;

  for(int test = 0; test < TEST_BATCH_COUNT; ++test) {
    cout<<"Running batch "<<test<<"..."<<endl;

    auto beginTime = chrono::high_resolution_clock::now();
    for(int i = 0; i < TEST_BATCH; ++i)
      testOnce();
    auto endTime = chrono::high_resolution_clock::now();

    chrono::duration<double, milli> dur = endTime - beginTime;
    cout<<"Batch "<<test<<" finished."<<endl;
    cout<<"Time: "<<dur.count()<<"ms"<<endl<<endl;

    totalTimeCount += dur.count();
  }

  cout<<"---All Complete---"<<endl;
  cout<<"Avg. time per query: "<<totalTimeCount / (TEST_BATCH * TEST_BATCH_COUNT)<<"ms"<<endl;
}
