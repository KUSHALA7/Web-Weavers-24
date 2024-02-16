"use strict";;
Object.defineProperty(exports, "__esModule", { value: true });
var tslib_1 = require("tslib");
var es2016_1 = (0, tslib_1.__importDefault)(require("./operators/es2016"));
var es6_1 = (0, tslib_1.__importDefault)(require("./es6"));
function default_1(fork) {
    // The es2016OpsDef plugin comes before es6Def so BinaryOperators and
    // AssignmentOperators will be appropriately augmented before they are first
    // used in the core definitions for this fork.
    fork.use(es2016_1.default);
    fork.use(es6_1.default);
}
exports.default = default_1;
module.exports = exports["default"];
