#!/usr/bin/node 

var http = require( 'http' );
var fs   = require( 'fs' );
var util = require( 'util' );
var optM = require( 'node-getopt' );

var args = [
	[ 'h', 'help',              'Print this help.' ],
	[ 'p', 'port=i',            'Port to serve on.' ],
	[ 'd', 'data=ARG',          'File to read for stats data.' ],
	[ 'm', 'metrics=ARG',       'File to read for metrics data.' ],
	[ 'j', 'json=ARG',          'File to read for json data.' ],
];

var data = {
	'/slack':		[ 'ok', { } ],
	'/data':		[ null, { } ],
	'/metrics':		[ null, { } ],
	'/json':        [ null, { 'Content-Type': 'application/json' } ],
	'/':			[ '', { } ],
};

var port = 10000;


var logmsg = function( msg ) {
	console.log( '[' + ((new Date).getTime( ) / 1000).toFixed( 3 ) + '] ' + msg );
};

var logobj = function( obj ) {
	logmsg( 'Object:\n' + util.inspect( obj, {
		showHidden: 	true,
		customInspect:	true,
		colors:			true,
		depth:			6,
	}));
};

var respond = function( req, res ) {

	var sent = '';

	if( !( req.url in data ) ) {
		logmsg( 'Unexpected path: ' + req.url );
		res.writeHead( 404 );
		res.end( );
		return;
	}

	req.on( 'data', function( chunk ) {
		sent += chunk.toString( );
	});

	req.on( 'end', function( ) {
		if( sent.length > 0 ) {
			var o;

			try {
				o = JSON.parse( sent );
			} catch( err ) {
				logmsg( 'Invalid JSON posted: ' + err.toSring( ) );
				return;
			}

			logobj( o );
		}

		var arr = data[req.url];
		var code = 200;
		var hdr = { };
		if( arr[0].length ) {
			hdr['Content-Length'] = arr[0].length;
		} else {
			code = 204;
		}
		for( var h in arr[1] ) {
			hdr[h] = arr[1][h];
		}

		logmsg( 'Serving ' + req.url );
		res.writeHead( code, hdr );
		res.write( arr[0] );
		res.end( );
	});
};


var opt = optM.create( args );
opt.setHelp(
	"Usage: node data_server.js --help\n" +
	"       node data_server.js [--port <port>] [--data <stats file>] [--metrics <metrics file>]\n\n" +
	"This small webserver servers static stats/metrics content for testing ministry fetch.\n\n" );

var opts = opt.bindHelp( ).parseSystem( ).options;

if( opts.metrics ) {
	data['/metrics'][0] = fs.readFileSync( opts.metrics );
	logmsg( `/metrics taken from ${opts.metrics}.` ); 
}
if( opts.data ) {
	data['/data'][0] = fs.readFileSync( opts.data );
	logmsg( `/data taken from ${opts.data}.` );
}
if( opts.json ) {
	data['/json'][0] = fs.readFileSync( opts.json );
	logmsg( `/json take from ${opts.json}.` );
}
if( opts.port ) {
	port = parseInt( opts.port, 10 );
}

var str = '';
for( var p in data ) {
	if( data[p][0] ) {
		str += p + "\n";
	}
}
data['/'][0] = str;

var svr = http.createServer( respond );
svr.listen( port, function( ) {
	logmsg( `Server listening on port ${port}.` );
});

