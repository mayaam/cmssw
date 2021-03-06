#ifndef PopConBTransitionSourceHandler_H
#define PopConBTransitionSourceHandler_H

#include "CondCore/CondDB/interface/ConnectionPool.h"
#include "CondCore/CondDB/interface/IOVProxy.h"
#include "CondCore/PopCon/interface/PopConSourceHandler.h"
#include "CondFormats/RunInfo/interface/RunInfo.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include <string>

namespace popcon {
  template <class T>
  class PopConESTransitionSourceHandler: public PopConSourceHandler<T> {
  public:
    PopConESTransitionSourceHandler( edm::ParameterSet const & pset ):
      m_run( pset.getParameter<edm::ParameterSet>( "ESTransition" ).getParameter<unsigned long long>( "runNumber" ) ),
	//      m_currentThreshold( pset.getParameter<edm::ParameterSet>( "ESTransition" ).getUntrackedParameter<double>( "currentThreshold", 18000. ) ),
      m_tagForRunInfo( pset.getParameter<edm::ParameterSet>( "ESTransition" ).getParameter<std::string>( "tagForRunInfo" ) ),
      m_ESGain( pset.getParameter<edm::ParameterSet>( "ESTransition" ).getParameter<std::string>( "ESGain" ) ),
      m_tagForLowGain( pset.getParameter<edm::ParameterSet>( "ESTransition" ).getParameter<std::string>( "ESLowGainTag" ) ),
      m_tagForHighGain( pset.getParameter<edm::ParameterSet>( "ESTransition" ).getParameter<std::string>( "ESHighGainTag" ) ),
      m_connectionString( pset.getParameter<edm::ParameterSet>( "ESTransition" ).getParameter<std::string>( "connect" ) ),
      m_connectionPset( pset.getParameter<edm::ParameterSet>( "ESTransition" ).getParameter<edm::ParameterSet>( "DBParameters" ) ) {
      edm::LogInfo( "PopConESTransitionSourceHandler" ) << "[" << "PopConESTransitionSourceHandler:" << __func__ << "]: "
                                                       << "Initialising Connection Pool" << std::endl;
      m_connection.setParameters( m_connectionPset );
      m_connection.configure();
  }

    ~PopConESTransitionSourceHandler() override {}

    std::string id() const final { return std::string( "PopConESTransitionSourceHandler" ); }

    bool checkLowGain() {
      //the output boolean is set to true as default
      bool isLowGain = true;
      cond::persistency::Session& session = PopConSourceHandler<T>::dbSession();
      //reading RunInfo from Conditions
      cond::persistency::TransactionScope trans( session.transaction() );
      edm::LogInfo( "PopConESTransitionSourceHandler" ) << "[" << "PopConESTransitionSourceHandler::" << __func__ << "]: "
                                                       << "Loading tag for RunInfo " << m_tagForRunInfo
                                                       << " and IOV valid for run number: " << m_run << std::endl;
      cond::persistency::IOVProxy iov = session.readIov( m_tagForRunInfo );
      cond::Iov_t currentIov = *(iov.find( m_run ));
      LogDebug( "PopConESTransitionSourceHandler" ) << "Loaded IOV sequence from tag " << m_tagForRunInfo
                                                   << " with size: "<< iov.loadedSize()
                                                   << ", IOV valid for run number " << m_run << " starting from: " << currentIov.since
                                                   << ", with corresponding payload hash: " << currentIov.payloadId << std::endl;
							   /*    accessing the average magnet current for the run
            double current_default = -1;
           double avg_current = current_default;
           avg_current = session.fetchPayload<RunInfo>( currentIov.payloadId )->m_avg_current;
            LogDebug( "PopConESTransitionSourceHandler" ) << "Comparing value of magnet current: " << avg_current << " A for run: " << m_run
                                                   << " with the corresponding threshold: "<< m_currentThreshold << " A." << std::endl;
      comparing the magnet current with the user defined threshold
      if( avg_current != current_default && avg_current <= m_currentThreshold ) isLowGain = false;
      edm::LogInfo( "PopConESTransitionSourceHandler" ) << "[" << "PopConESTransitionSourceHandler::" << __func__ << "]: "
                                                       << "The magnet was " << ( isBOn ? "ON" : "OFF" )
						       << " during run " << m_run << std::endl;   */
      if (m_ESGain=="HIGH") {
	isLowGain=false;}
	  else   {
	    isLowGain = true;}
      trans.close();          
      return isLowGain;
    }

    virtual void getObjectsForESTransition( bool isLowGain ) {
      //reading payloads for 0T and 3.8T from Conditions
      cond::persistency::Session& session = PopConSourceHandler<T>::dbSession();
      cond::persistency::TransactionScope trans( session.transaction() );
      edm::LogInfo( "PopConESTransitionSourceHandler" ) << "[" << "PopConESTransitionSourceHandler::" << __func__ << "]: "
                                                       << "Loading tag for ES " << ( isLowGain ? "LowGain" : "HighGain" ) << ": "
                                                       << ( isLowGain ? m_tagForLowGain : m_tagForHighGain )
                                                       << " and IOV valid for run number: " << m_run << std::endl;
      cond::persistency::IOVProxy iov = session.readIov( isLowGain ? m_tagForLowGain : m_tagForHighGain, true );
      cond::Iov_t currentIov = *(iov.find( m_run ));
      LogDebug( "PopConESTransitionSourceHandler" ) << "Loaded IOV sequence from tag " << ( isLowGain ? m_tagForLowGain : m_tagForHighGain )
                                                   << " with size: "<< iov.loadedSize()
                                                   << ", IOV valid for run number " << m_run << " starting from: " << currentIov.since
                                                   << ", with corresponding payload hash: " << currentIov.payloadId
                                                   << std::endl;
      std::string destTag = this->tagInfo().name;
      if( currentIov.payloadId != this->tagInfo().lastPayloadToken ) {
        std::ostringstream ss;
        ss << "Adding iov with since "<<m_run<<" pointing to hash " << currentIov.payloadId
           << " corresponding to the ES Gain "
           << ( isLowGain ? "LOW" : "HIGH" );
        edm::LogInfo( "PopConESTransitionSourceHandler" ) << "[" << "PopConESTransitionSourceHandler::" << __func__ << "]: "
                                                         << ss.str() << std::endl;
	cond::persistency::IOVEditor editor;
        if( session.existsIov( destTag ) ){
	  editor = session.editIov( destTag );
	} else {
	  editor = session.createIov<T>( destTag, iov.timeType() );
          editor.setDescription( "Tag created by PopConESTransitionSourceHandler" );
	}
        editor.insert( m_run, currentIov.payloadId );
        editor.flush();
        this->m_userTextLog = ss.str();
      } else {
        edm::LogInfo( "PopConESTransitionSourceHandler" ) << "[" << "PopConESTransitionSourceHandler::" << __func__ << "]: "
                                                         << "The payload with hash " << currentIov.payloadId
                                                         << " corresponding to ES Gain"
                                                         << ( isLowGain ? "LOW" : "HIGH" )
                                                         << " is still valid for run " << m_run
                                                         << " in the destination tag " << destTag
                                                         << ".\nNo transfer needed." <<std::endl;
      }
      trans.close();     
    }

    void getNewObjects() final {
      //check whats already inside of database
       edm::LogInfo( "PopConESTransitionSourceHandler" ) << "[" << "PopConESTransitionSourceHandler::" << __func__ << "]: "
                                                        << "Destination Tag Info: name " << this->tagInfo().name
                                                        << ", size " << this->tagInfo().size
                                                        << ", last object valid since " << this->tagInfo().lastInterval.first
                                                        << ", hash " << this->tagInfo().lastPayloadToken << std::endl;
      //check if a transfer is needed:
      //if the new run number is smaller than or equal to the latest IOV, exit.
      //This is needed as now the IOV Editor does not always protect for insertions:
      //ANY and VALIDATION sychronizations are allowed to write in the past.
      if( this->tagInfo().size > 0  && this->tagInfo().lastInterval.first >= m_run ) {
        edm::LogInfo( "PopConESTransitionSourceHandler" ) << "[" << "PopConESTransitionSourceHandler::" << __func__ << "]: "
                                                         << "last IOV " << this->tagInfo().lastInterval.first
                                                         << ( this->tagInfo().lastInterval.first == m_run ? " is equal to" : " is larger than" )
                                                         << " the run proposed for insertion " << m_run
                                                         << ". No transfer needed." << std::endl;
        return;
      }
      bool isLowGain = checkLowGain();
      getObjectsForESTransition( isLowGain );
      edm::LogInfo( "PopConESTransitionSourceHandler" ) << "[" << "PopConESTransitionSourceHandler::" << __func__ << "]: "
                                                       << "END." << std::endl;
    }

  private:
    unsigned long long m_run;
    double m_currentThreshold;
    // for reading from CondDB the current from RunInfo
    std::string m_tagForRunInfo;
    // for reading from CondDB the Low Gain and High Gain payloads
    std::string m_ESGain;
    std::string m_tagForLowGain;
    std::string m_tagForHighGain;
    std::string m_connectionString;
    edm::ParameterSet m_connectionPset;
    cond::persistency::ConnectionPool m_connection;
  };
} //namespace popcon

#endif //PopConESTransitionSourceHandler_H
