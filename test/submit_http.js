#!/usr/bin/env node

var https = require( 'https' );
var opts  = {
    host:   'localhost',
    port:   9443,
    path:   '/adder',
    method: 'POST',
};

var data = [
    'foo.bar 1',   
    'foo.eek 2'
];

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

var str = data.join( '\n' ) + '\n';

req.setHeader( 'Content-Type', 'text/plain' );
req.setHeader( 'Content-Length', str.length );

req.write( str, 'utf8' );
req.end( );

