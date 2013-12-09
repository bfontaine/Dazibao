(function(body) {
    var api = {
        call: function(method, path, callback) {
            var xhr = new XMLHttpRequest();
            xhr.open(method, path);
            xhr.onreadystatechange = function() {
                if (xhr.readyState == 4) {
                    callback(xhr);
                }
            };
            xhr.send(null);
        },
        del: function(tlv_el) {
            var off = +tlv_el.getAttribute('data-offset');
            if (isNaN(off)) { return; }
            api.call('post', '/tlv/delete/'+off, function(xhr) {
                if (xhr.status == 204) {
                    tlv_el.parentElement.removeChild(tlv_el);
                } else {
                    console.log("Error while deleting TLV at offset "+off, xhr);
                }
            });
        }
    };


    /* 'delete' button */
    var tlvs = document.getElementsByClassName('tlv'),
        actions = '<ul class="actions"><li class="delete">Delete</li></ul>';

    for (var i=0, l=tlvs.length; i<l; i++) {
        tlvs[i].innerHTML += actions;
    }

    body.addEventListener('click', function(e) {
        var el = e.target, tlv, off;
        console.log(_e = el);
        if (el.className == 'delete') {
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
            b.innerHTML = res[1] + '<br/><br/><span class="author">'
                        + '<span><span class="sep">—</span> '
                        + res[2] + '</span></span>';
        }
    }

})(document.getElementsByTagName('body')[0]);
