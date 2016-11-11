var https = require( 'https' );
var net   = require( 'net' );

var opts  = {
    host:       'localhost',
    port:       9443,
    path:       '/tokens',
    method:     'GET',
};

var dict = {
	first: [
		'apache',
		'nginx',
		'tomcat',
		'varnish',
		'mongo',
		'redis',
		'memcached',
		'mysql',
		'couchbase',
		'hornetq',
		'rabbitmq',
		'sendmail',
		'syslog',
	],
	second: [
		'uptime',
		'memory',
		'connections',
		'errors',
		'times',
		'locks',
		'requests',
	],
	third: [
		'webhost',
		'dbhost',
		'apphost',
	],
};

function sendSet( sk ) {

	var x, y, z, i, k = 0;

	for( x = 0; x < dict.first.length; x++ ) {
		for( y = 0; y < dict.second.length; y++ ) {
			for( z = 0; z < dict.third.length; z++, k++ ) {
				sk.write( dict.first[x] + '.' + dict.second[y] + '.' + dict.third[z] + ' ' + k + '\n' );
			}
		}
	}
}



var connect_with_token = function( tokens ) {

    var path = new net.Socket( );
    var stat = new net.Socket( );

    console.log( tokens );

    stat.connect( 9125, '127.0.0.1', function( ) {
        stat.write( tokens.stats + '\n' );
        setInterval( sendSet, 10, stat );
    });

    path.connect( 9225, '127.0.0.1', function( ) {
        path.write( tokens.adder + '\n' );
        setInterval( sendSet, 29, path );
    });
};


process.env.NODE_TLS_REJECT_UNAUTHORIZED = 0;

https.get( opts, function( res ) {

    var content = '';
    var tokens;

    res.on( 'data', function( chunk ) {
        content += chunk.toString( );
    });

    res.on( 'end', function( ) {
        try {
            tokens = JSON.parse( content );
            connect_with_token( tokens );
        } catch( err ) {
            console.log( 'Could not handle tokens response: ' + content );
            process.exit( 0 );
        }
    });
});

