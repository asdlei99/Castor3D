#include "CastorUtils/Config/PlatformConfig.hpp"

#if defined( CU_PlatformAndroid )

#include "CastorUtils/Data/File.hpp"

#include "CastorUtils/Math/Math.hpp"
#include "CastorUtils/Miscellaneous/Utils.hpp"
#include "CastorUtils/Log/Logger.hpp"

#	include <cstdio>
#	include <cstring>
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <iostream>
#	include <unistd.h>
#	include <sys/types.h>
#	include <dirent.h>
#	include <errno.h>
#	include <pwd.h>

namespace castor
{
	bool File::traverseDirectory( Path const & folderPath
		, TraverseDirFunction directoryFunction
		, HitFileFunction fileFunction )
	{
		CU_Require( !folderPath.empty() );
		bool result = false;
		DIR * dir;

		if ( ( dir = opendir( string::stringCast< char >( folderPath ).c_str() ) ) == nullptr )
		{
			switch ( errno )
			{
			case EACCES:
				Logger::logWarning( cuT( "Can't open dir : Permission denied - Directory : " ) + folderPath );
				break;

			case EBADF:
				Logger::logWarning( cuT( "Can't open dir : Invalid file descriptor - Directory : " ) + folderPath );
				break;

			case EMFILE:
				Logger::logWarning( cuT( "Can't open dir : Too many file descriptor in use - Directory : " ) + folderPath );
				break;

			case ENFILE:
				Logger::logWarning( cuT( "Can't open dir : Too many files currently open - Directory : " ) + folderPath );
				break;

			case ENOENT:
				Logger::logWarning( cuT( "Can't open dir : Directory doesn't exist - Directory : " ) + folderPath );
				break;

			case ENOMEM:
				Logger::logWarning( cuT( "Can't open dir : Insufficient memory - Directory : " ) + folderPath );
				break;

			case ENOTDIR:
				Logger::logWarning( cuT( "Can't open dir : <name> is not a directory - Directory : " ) + folderPath );
				break;

			default:
				Logger::logWarning( cuT( "Can't open dir : Unknown error - Directory : " ) + folderPath );
				break;
			}

			result = false;
		}
		else
		{
			result = true;
			dirent * dirent;

			while ( result && ( dirent = readdir( dir ) ) != nullptr )
			{
				String name = string::stringCast< xchar >( dirent->d_name );

				if ( name != cuT( "." ) && name != cuT( ".." ) )
				{
					if ( dirent->d_type == DT_DIR )
					{
						result = directoryFunction( folderPath / name );
					}
					else
					{
						fileFunction( folderPath, name );
					}
				}
			}

			closedir( dir );
		}

		return result;
	}

	bool fileOpen( FILE *& p_file, std::filesystem::path const & p_path, char const * p_mode )
	{
		p_file = fopen( p_path.c_str(), p_mode );
		return p_file != nullptr;
	}

	bool fileOpen64( FILE *& p_file, std::filesystem::path const & p_path, char const * p_mode )
	{
		p_file = fopen( p_path.c_str(), p_mode );
		return p_file != nullptr;
	}

	bool fileSeek( FILE * p_file, int64_t p_offset, int p_iOrigin )
	{
		return fseek( p_file, size_t( p_offset ), p_iOrigin ) == 0;
	}

	int64_t fileTell( FILE * p_file )
	{
		return ftell( p_file );
	}

	Path File::getExecutableDirectory()
	{
		Path pathReturn;
		char path[FILENAME_MAX];
		char buffer[32];
		sprintf( buffer, "/proc/%d/exe", getpid() );
		int bytes = int( std::min( size_t( readlink( buffer, path, sizeof( path ) ) ), sizeof( path ) - 1 ) );

		if ( bytes > 0 )
		{
			path[bytes] = '\0';
			pathReturn = Path{ string::stringCast< xchar >( path ) };
		}

		pathReturn = pathReturn.getPath();
		return pathReturn;
	}

	Path File::getUserDirectory()
	{
		Path pathReturn;
		struct passwd * pw = getpwuid( getuid() );
		const char * homedir = pw->pw_dir;
		pathReturn = Path{ string::stringCast< xchar >( homedir ) };

		return pathReturn;
	}

	bool File::directoryExists( Path const & p_path )
	{
		struct stat status = {};
		stat( string::stringCast< char >( p_path ).c_str(), &status );
		return ( status.st_mode & S_IFDIR ) == S_IFDIR;
	}

	bool File::directoryCreate( Path const & p_path, FlagCombination< CreateMode > const & p_flags )
	{
		Path path = p_path.getPath();

		if ( !path.empty() && !directoryExists( path ) )
		{
			directoryCreate( path, p_flags );
		}

		mode_t mode = 0;

		if ( checkFlag( p_flags, CreateMode::eUserRead ) )
		{
			mode |= S_IRUSR;
		}

		if ( checkFlag( p_flags, CreateMode::eUserWrite ) )
		{
			mode |= S_IWUSR;
		}

		if ( checkFlag( p_flags, CreateMode::eUserExec ) )
		{
			mode |= S_IXUSR;
		}

		if ( checkFlag( p_flags, CreateMode::eGroupRead ) )
		{
			mode |= S_IRGRP;
		}

		if ( checkFlag( p_flags, CreateMode::eGroupWrite ) )
		{
			mode |= S_IWGRP;
		}

		if ( checkFlag( p_flags, CreateMode::eGroupExec ) )
		{
			mode |= S_IXGRP;
		}

		if ( checkFlag( p_flags, CreateMode::eOthersRead ) )
		{
			mode |= S_IROTH;
		}

		if ( checkFlag( p_flags, CreateMode::eOthersWrite ) )
		{
			mode |= S_IWOTH;
		}

		if ( checkFlag( p_flags, CreateMode::eOthersExec ) )
		{
			mode |= S_IXOTH;
		}

		return mkdir( string::stringCast< char >( p_path ).c_str(), mode ) == 0;
	}

	bool File::deleteEmptyDirectory( Path const & p_path )
	{
		bool result = rmdir( string::stringCast< char >( p_path ).c_str() ) == 0;

		if ( !result )
		{
			auto error = errno;

			switch ( error )
			{
			case ENOTEMPTY:
				Logger::logError( makeStringStream() << cuT( "Couldn't remove directory [" ) << p_path << cuT( "], it is not empty." ) );
				break;

			case ENOENT:
				Logger::logError( makeStringStream() << cuT( "Couldn't remove directory [" ) << p_path << cuT( "], the path is invalid." ) );
				break;

			case EACCES:
				Logger::logError( makeStringStream() << cuT( "Couldn't remove directory [" ) << p_path << cuT( "], a program has an open handle to this directory." ) );
				break;

			default:
				Logger::logError( makeStringStream() << cuT( "Couldn't remove directory [" ) << p_path << cuT( "], unknown error (" ) << error << cuT( ")." ) );
				break;
			}
		}

		return result;
	}
}

#endif
