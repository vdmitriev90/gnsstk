#ifndef POD_SOLUTION_DATABASE
#define POD_SOLUTION_DATABASE

#include "GnssEpochMap.h"
#include "SQLiteConnection.h"

#include <cstdint>
#include <functional>
#include <string>

namespace pod
{
	// Per-rover solution database: GnssObsFile, Epochs, SlnDataItems, SvDataItems and their lookup tables.
	class SolutionDatabase : public SQLiteConnection
	{
	  public:
		static const unsigned SCHEMA_VERSION;
		static const std::int32_t APPLICATION_ID;

		explicit SolutionDatabase(const std::string& path);

		// Creates a new database holding exactly one solution. Throws if 'finalPath' already exists.
		// Data is written to <finalPath>.partial, committed, the connection closed, and only then is the
		// file renamed to 'finalPath'. On any error the partial file is removed and nothing is published.
		static void create(const std::string& finalPath, const GnssEpochMap& eMap);
		static void create(const std::string& finalPath, const std::function<void(SolutionDatabase&)>& writer);

		// Atomically stores file metadata and all epochs; returns the new GnssObsFile.ID.
		// Nothing is written if any part fails.
		std::int64_t addSolution(const GnssEpochMap& eMap);

	  private:
		void setPragmas();
		void migrate();

		std::int64_t addFileMetadata(const GnssEpochMap& eMap);
		void addEpochs(const GnssEpochMap& eMap);

		void addObsData(const std::pair<gnsstk::TypeID, double>& typeValuePair);
		void addSlnData(const gnsstk::typeValueMap& slnData);
		void addSvData(const gnsstk::satTypeValueMap& svData);
		void addNewEpoch(const std::pair<gnsstk::CommonTime, GnssEpoch>& epoch);
		std::int64_t addSV(const gnsstk::SatID& sv);

		std::int64_t lastFileID_ = 0;
		std::int64_t lastEpochID_ = 0;
		std::int64_t lastTypeValuePairID_ = 0;
		int obsItemCounter_ = 0;
		int maxObsItemsPerTransaction_ = 100000;
	};
} // namespace pod

#endif // !POD_SOLUTION_DATABASE
