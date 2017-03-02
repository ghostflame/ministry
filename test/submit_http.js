#!/usr/bin/env node

var https = require( 'https' );
var opts  = {
    host:   'localhost',
    port:   9443,
    path:   '/adder',
    method: 'POST',
};

process.env.NODE_TLS_REJECT_UNAUTHORIZED = 0;

var req = https.request( opts, function( res ) {

    var resp = '';

    res.on( 'data', function( chunk ) {
        resp += chunk.toString( );
    });

    res.on( 'end', function( ) {
        console.log( 'Got response: ' + resp );
        process.exit( 0 );
    });
});

req.on( 'error', function( err ) {
    console.log( 'Request error: ' + err );
    process.exit( 1 );
});


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

var k = 0;
var data = [ ];
for( x = 0; x < dict.first.length; x++ ) {
	for( y = 0; y < dict.second.length; y++ ) {
		for( z = 0; z < dict.third.length; z++ ) {
			for( i = 10; i < 99; i++, k++ ) {
				data.push( dict.first[x] + '.' + dict.second[y] + '.' + dict.third[z] + i + ' ' + k );
			}
		}
	}
}

var str = data.join( '\n' ) + '\n';

req.setHeader( 'Content-Type', 'text/plain' );
req.setHeader( 'Content-Length', str.length );

req.write( str, 'utf8' );
req.end( );

console.log( "Sent " + data.length + " lines, " + str.length + " bytes." );

