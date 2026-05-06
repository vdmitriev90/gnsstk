#pragma once
#include<string>
#include"ProcessingClass.hpp"
namespace pod
{
    class CatcherStatistic
    {
    public:
        CatcherStatistic(std::string path)
            :file(path)
        {
        };

        ~CatcherStatistic() {};

        void logStatistic(const std::list<gnsstk::ProcessingClass*> &) const;
        
        struct dataRecord
        {
            dataRecord(std::string cname, std::set<gnsstk::SatID> svSet) :
                className(cname), satRejected(svSet) 
            {};

            std::string className;
            std::set<gnsstk::SatID> satRejected;
        };

        typedef std::list< dataRecord> dataTableItem;
        typedef std::map<gnsstk::CommonTime, dataTableItem> dataTable;

    private:
        std::string file;
    };
}

