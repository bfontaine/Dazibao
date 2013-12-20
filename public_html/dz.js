(function(cb, main) {
    var xhr = new XMLHttpRequest();
    xhr.open('get', '/js_tpls');
    xhr.onreadystatechange = function() {
        if (xhr.readyState == 4) {
            cb(xhr.responseText, main);
        }
    };
    xhr.send(null);
})(function(strs, main) {
    /* basic templating system. Writing HTML in templates is easier than
     * adding it in html.h and compile everything again */

    var tpls = {},
        re_first_nl = /\n([\s\S]+)/,
        re_tpl_var = /%\{(\w+)\}/g;

    strs = strs.split(/\n+--\n+/);

    for (var i=0, l=strs.length, parts; i<l; i++) {
        parts = strs[i].split(re_first_nl);
        tpls[parts[0]] = parts[1];
    }

    /* tpl('<p>%{name}</p>', {name: 'foo'}) -> '<p>foo</p>' */
    window.tpl = function(name, params) {
        if (!tpls.hasOwnProperty(name)) {
            return '';
        }

        if (!params) { return tpls[name]; }

        return tpls[name].replace(re_tpl_var, function(_, v) {
            return params.hasOwnProperty(v) ? params[v] : '';
        });
    };

    main(document.getElementsByTagName('body')[0]);
},
function(body) {
    /* Note: the code here is not meant to be perfect, we're not doing a Web
     * project so we'll avoid spending too much time in JS code. */

    var api = {
        call: function(method, path, data, callback) {
            var xhr = new XMLHttpRequest();
            xhr.open(method, path);
            xhr.onreadystatechange = function() {
                if (xhr.readyState == 4) {
                    callback(xhr);
                }
            };
            xhr.send(data);
        },
        del: function(tlv_el) {
            var off = +tlv_el.getAttribute('data-offset');
            if (isNaN(off)) { return; }
            api.call('post', '/tlv/delete/'+off, null, function(xhr) {
                if (xhr.status == 204) {
                    tlv_el.parentElement.removeChild(tlv_el);
                } else {
                    console.log("Error while deleting TLV at offset "+off, xhr);
                }
            });
        },
        compact: function(callback) {
            api.call('post', '/compact', null, function(xhr) {
                callback(xhr.status == 200 ? +xhr.responseText : -1, xhr);
            });
        },
        hash: function(callback) {
            api.call('get', '/hash', null, function(xhr) {
                callback(xhr.status == 200 ? +xhr.responseText : -1, xhr);
            });
        },
        addText: function(text, callback) {
            api.call('post', '/tlv/add/text', text, function(xhr) {
                callback(xhr.status == 204);
            })
        },
        addTLV: function(data, callback) {
            api.call('post', '/tlv/add/form', data, function(xhr) {
                callback(xhr);
            });
        }
    };


    /* 'delete' button */
    var tlvs = document.getElementsByClassName('tlv'),
        actions = '<ul class="actions"><li class="deleteTLV">Delete</li></ul>';

    for (var i=0, l=tlvs.length; i<l; i++) {
        tlvs[i].innerHTML += actions;
    }

    body.addEventListener('click', function(e) {
        var el = e.target, tlv, off;
        if (el.className == 'deleteTLV') {
            tlv = el.parentElement.parentElement;
            api.del(tlv);
        }
    }, false);

    /* quotes in text TLVs */
    var quote_re = /(.*?)\s*--\s+((?:[^-\.]|-[^-])+)$/;

    for (var i=0, l=tlvs.length; i<l; i++) {
        if (+tlvs[i].getAttribute('data-type') != 2) {
            continue;
        }
        var b = tlvs[i].getElementsByTagName('blockquote')[0],
            res = quote_re.exec(b.innerHTML);

        if (res && res.length == 3) {
            b.innerHTML = tpl('quote', {
                text: res[1],
                author: res[2]
            });
        }
    }

    /* Dazibao settings */
    !function() {
        var html = tpl('settings'), actions, buttons = {};

        document.body.innerHTML += html;
        actions = document.querySelector('#settings .actions');

        function addButton( text, id, callback ) {
            var b = document.createElement('li');
            id = '_' + id;
            b.className = id;
            buttons[id] = callback;
            b.innerText = b.textContent = text;
            actions.appendChild(b);
        }

        actions.addEventListener('click', function( e ) {
            var id = e.target.className;
            if (buttons[id]) {
                buttons[id](e.target);
            }
        }, false);

        /* -- compacting -- */
        addButton('Compact', 'cpct', function() {
            api.compact(function( saved, xhr ) {
                if (saved < 0) {
                    alert("An error occured while compacting a Dazibao");
                    console.log(xhr);
                } else {
                    alert("Saved " + saved + " bytes.");
                }
            });
        });

        /* -- adding a text TLV -- */
        addButton('Add a text', 'addtlv', function() {
            var text = prompt("Text?");

            api.addText(text, function( ok ) {
                alert(ok ? "ok, refresh!" : "oh no, an error :(");
            });
        });

        /* -- adding a TLV -- */
        addButton('Add a file', 'addtlv', function() {
            //
        });
    }();

    /* Notifications */
    !function() {

        var prev_hash = 0,
            min_timeout = 2000,
            max_timeout = 30000,
            timeout = 10000; // 10 seconds

        function check_hash() {
            api.hash(function( hash ) {
                var changed = false;

                if (prev_hash != 0) {
                    changed = hash > -1 && (hash != prev_hash);
                }
                prev_hash = hash;

                if (changed) {
                    timeout = Math.max(min_timeout, timeout/2);
                    alert("changed!"); // TODO better notification
                    console.log("Dazibao changed. Checking again in "
                                + timeout);
                } else {
                    timeout = Math.min(max_timeout, timeout*1.5);
                    console.log("Dazibao didn't change. Checking again in "
                                + timeout);
                }

                setTimeout(check_hash, timeout);
            });
        }

        check_hash();
    }();

});
