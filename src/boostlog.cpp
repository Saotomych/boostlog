//============================================================================
// Name        : boostlog.cpp
// Author      : Non nobis, Domine, non nobis, sed nomini tuo da gloriam.
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================


#define BOOST_LOG_DYN_LINK 1

#include <iostream>
#include <string>
#include <iomanip>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_multifile_backend.hpp>

#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/logger.hpp>

#include <boost/log/expressions.hpp>
#include <boost/log/expressions/attr_fwd.hpp>
#include <boost/log/expressions/attr.hpp>

#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/attribute_cast.hpp>
#include <boost/log/attributes/attribute_value.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/attributes/counter.hpp>

#include <boost/log/support/date_time.hpp>

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

using namespace std;

namespace logging = boost::log;
namespace trivial = logging::trivial;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;
namespace gregorian = boost::gregorian;

// Логгер
// Много файлов в дебаге
// Таймштамп в имени файла
// Номер в имени файла при релизе
// Обрезка файла по времени и размеру

// We define our own severity levels
enum severity_level
{
    trace,
	debug,
    warning,
    error,
	info,
    critical
};

namespace
{
	src::severity_logger_mt< severity_level > lg;

	severity_level filterLevel = trace;
	int logSize = 1024*1024;
	std::string logFileName = "release_%2N_%d.%m.%Y-%H:%M:%S.log";
}

// The operator puts a human-friendly representation of the severity level to the stream
std::ostream& operator<< (std::ostream& strm, severity_level level)
{
    static const char* strings[] =
    {
        "trace",
		"debug",
        "warning",
        "error",
        "info",
        "critical"
    };

    if (static_cast< std::size_t >(level) < sizeof(strings) / sizeof(*strings))
        strm << strings[level];
    else
        strm << static_cast< int >(level);

    return strm;
}

BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", severity_level)

void init(severity_level level)
{

	boost::log::core::get()->set_filter(severity >= level);

}

class ToIntegerConvertor
{
	int value;

public:

	ToIntegerConvertor(): value(0) {}
	ToIntegerConvertor(int v): value(v) {}
	ToIntegerConvertor( const std::string& op2 )
	{
		value = stoi(op2);
	}

	ToIntegerConvertor& operator=( const std::string& op2 )
	{
		value = stoi(op2);
		return *this;
	}

	int v()
	{
		return value;
	}

};

template<typename T>
T getArg(std::string str)
{
	T res(str);
	return res;
}

template<>
boost::log::trivial::severity_level getArg<boost::log::trivial::severity_level> (std::string str)
{
	if (str == "trace") return boost::log::trivial::trace;
	if (str == "debug") return boost::log::trivial::debug;
	if (str == "info") return boost::log::trivial::info;
	if (str == "warning") return boost::log::trivial::warning;
	if (str == "error") return boost::log::trivial::error;
	if (str == "fatal") return boost::log::trivial::fatal;
	return boost::log::trivial::trace;
}

template<>
severity_level getArg<severity_level> (std::string str)
{
	if (str == "trace") return severity_level::trace;
	if (str == "debug") return severity_level::debug;
	if (str == "warning") return severity_level::warning;
	if (str == "error") return severity_level::error;
	if (str == "info") return severity_level::info;
	if (str == "critical") return severity_level::critical;
	return severity_level::trace;
}

void initDebugLogging()
{
    boost::shared_ptr< logging::core > core = logging::core::get();

    boost::shared_ptr< sinks::text_multifile_backend > backend =
		boost::make_shared< sinks::text_multifile_backend >();

    // Set up the file naming pattern
    backend->set_file_name_composer
    (
        sinks::file::as_file_name_composer(expr::stream << "logs/"
        		<< expr::attr< std::string >("RequestID")
				<< expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "_%d-%m-%Y_%H:%M:%S")
				<< ".log"
				)
    );

    // Wrap it into the frontend and register in the core.
    // The backend requires synchronization in the frontend.
    typedef sinks::synchronous_sink< sinks::text_multifile_backend > sink_t;
    boost::shared_ptr< sink_t > sink(new sink_t(backend));

    // Set the formatter
    sink->set_formatter
    (
        expr::stream
            << "[Log: " << expr::attr< std::string >("RequestID") << "] "
			<< std::setw(8) << std::setfill('0') << line_id << std::setfill(' ') << ": "
			<< expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%d-%m-%Y %H:%M:%S.%f")
			<< ": <" << severity << ">: "
			<< expr::smessage
    );

    core->add_sink(sink);

}

void initReleaseLogging()
{
	logging::add_file_log(
			keywords::target = "logs",
			keywords::file_name = logFileName,
			keywords::rotation_size = logSize * 1024,
			keywords::time_based_rotation = sinks::file::rotation_at_time_point(gregorian::greg_day(1)),
			keywords::format = (
			        expr::stream
						<< std::setw(8) << std::setfill('0') << line_id << std::setfill(' ') << ": "
						<< expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%d-%m-%Y %H:%M:%S")
						<< ": <" << severity << ">: "
						<< expr::smessage
			    ),
			keywords::auto_flush = true
	);

}

src::severity_logger_mt< severity_level >& getMyLogger()
{
	lg.add_attribute("LineCounter", attrs::counter< unsigned int >());

	return lg;
}

inline void CommonLog(severity_level level, std::string message)
{
	src::severity_logger_mt< severity_level >& lg = getMyLogger();
	BOOST_LOG_SEV(lg, level) << message;
}

inline void LocalLog(std::string name, severity_level level, std::string message)
{
	BOOST_LOG_SCOPED_THREAD_TAG("RequestID", name);
	src::severity_logger_mt< severity_level >& lg = getMyLogger();

    BOOST_LOG_SEV(lg, level) << message;
}


void SetToCommonLog(severity_level level, std::string message)
{
	std::cout << " lineid = " << line_id << std::endl;

#ifdef NDEBUG
	CommonLog(level, message);
#else
	LocalLog("debug", level, message);
#endif
}

void SetToLocalLog(std::string name, severity_level level, std::string message)
{
	std::cout << " lineid = " << line_id << std::endl;

#ifndef NDEBUG
	LocalLog(name, level, message);
#endif
   	SetToCommonLog(level, message);
}


int32_t LogParamParser(char* argv, char* argp)
{
	std::string argval(argv);
	int32_t len = 0;

	if ( argval == "--loglevel" || argval == "-l" )
	{
		filterLevel = getArg<severity_level>(argp);
		len=2;
	}

	if ( argval == "--logsize" || argval == "-s" )
	{
		logSize = getArg<ToIntegerConvertor>(argp).v();
		len=2;
	}

	if ( argval == "--logname" || argval == "-f" )
	{
		logFileName = getArg<std::string>(argp);
		len=2;
	}

	return len;
}

int main(int argc, char* argv[])
{

	uint32_t cnt = argc;
	uint32_t index = 1;
	while (cnt > 1)
	{
		int32_t len = 0;

		len = LogParamParser(argv[index], argv[index+1]);

//		if (len == 0)
//			len == // call next param parser

		if (len == 0)
			len = 1;

		index += len;
		cnt -= len;
	}

#ifdef NDEBUG
	initReleaseLogging();
#else
	initDebugLogging();
#endif

	init(filterLevel);
	logging::add_common_attributes();

	SetToCommonLog(debug, "Debug Test");
	SetToLocalLog("printer" , critical, "Car income");
	SetToCommonLog(info, "Info Test");
	SetToLocalLog("printer", trace, "Function end");
	SetToCommonLog(warning, "Warning Test");
	SetToLocalLog("shlagbaum_in", info, "Car registered");
	SetToCommonLog(trace, "Trace Test");
	SetToLocalLog("shlagbaum_in", critical, "Stack overflow");
	SetToCommonLog(critical, "Critical Test");


	return 0;
}
