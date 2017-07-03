/**
 * Created by helidium on 3/29/17.
 */
module.exports = (function() {
    "use strict";
    try { // NODE-GYP
        return require(`./bin/mitol`);
    } catch (e) {
        throw new Error('Error loading MitolServer module');
    }
})();