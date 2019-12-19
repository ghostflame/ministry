#!/usr/bin/node

var fs   = require( 'fs' );
var net  = require( 'net' );
var util = require( 'util' );
var http = require( 'http' );
var tls  = require( 'tls' );

// listens on 12003-12005 and reports counts

function PortCounter( port, show ) {

	var svr = null;
	var ctr = 0;

	var connHandler = function( sk ) {

		var lns, buf = '', cct = 0;

		var remote = sk.remoteAddress + ':' + sk.remotePort;

		console.log( 'New connection on port ' + port + ' from ' + remote );
		sk.setEncoding( 'utf8' );
		sk.on( 'data', function( d ) {

			buf += d.toString( );
			lns  = buf.split( '\n' );
			buf  = lns.pop( );
			ctr += lns.length;
			cct += lns.length;

			if( show ) {
				process.stdout.write( d );
			}
		});
		sk.on( 'close', function( ) {
			console.log( 'Connection from ' + remote + ' closed after ' + cct + ' lines.' );
		});
	};

	this.count = function( ) {
		return ctr;
	};

	if( port > 13000 ) {
		var opts = {
			key:	fs.readFileSync( '../dist/tls/key.pem', 'utf8' ),
			cert:	fs.readFileSync( '../dist/tls/cert.pem', 'utf8' ),
			passphrase: 'ministry',
		};
		svr = tls.createServer( opts,connHandler );
	} else {
		svr = net.createServer( connHandler );
	}
	svr.listen( port, '0.0.0.0', function( ) {
		console.log( 'Server listening on port ' + port + ( show ? ' (echoing).' : '.' ) );
	});
}


function PostHandler( port )
{
	var server = null;

	var handler = function( req, res ) {

		var obj, text = '';

		if( req.method !== 'POST' ) {
			res.writeHead( 405 );
			res.end( );
			return;
		}

		req.on( 'data', function(chunk) {
			text += chunk.toString( );
		});

		req.on( 'error', function( err ) {
			console.log( 'Request error: ' + err.toString( ) );
			res.writeHead( 501 );
			res.end( );
			return;
		});

		req.on( 'end', function( ) {
			try {
				obj = JSON.parse( text );
			} catch( err ) {
				res.writeHead( 400 );
				res.end( );
				return;
			}

			console.log( JSON.stringify( obj ) );
		});
	};

	server = http.createServer( handler );
	server.listen( port, '0.0.0.0', function( ) {
		console.log( 'Listening for HTTP connections on port ' + port + '.' );
	});
}


var ctrs = { };
var prts = [ 12003, 12004, 12005, 13003 ];
var totl = 0;
var show = false;

var echoport = parseInt( process.argv[2] || '13003', 10 );

for( var i = 0; i < prts.length; i++ ) {
	show = ( prts[i] === echoport );
	ctrs[prts[i]] = {
		ctr:	new PortCounter( prts[i], show ),
		sum:	0,
		prv:    0,
		lps:    0,
		prt:	prts[i],
	};
}

var postServer = new PostHandler( 13000 );

var printf = function( ) {
	console.log( util.format.apply( util, arguments ) );
};


var rate = function( prv, cur ) {
	return (( cur - prv ) / 10).toFixed( 3 );
};

var reporter = function( ) {

	var c, total = 0;
	var fmt = "    %s      %d     %s%%   (%s/s)";

	console.log( '[' + (~~ (Date.now()/1000)) + '] Received:' );

	for( var p in ctrs ) {
		c = ctrs[p];

		c.prv = c.sum;
		c.sum = c.ctr.count( );
		c.lps = rate( c.prv, c.sum );
		total += ctrs[p].sum;
	}

	for( var p in ctrs ) {
		c = ctrs[p];
		if( total === 0 ) {
			pct = 0;
		} else {
			pct = 100 * ( c.sum / total );
		}
		printf( fmt, p, c.sum, pct.toFixed( 2 ), c.lps );
	}

	printf( fmt, 'Total', total, '100.0', rate( totl, total ) );
	totl = total;
	console.log( '' );
};

setInterval( reporter, 10000 );





