#include <boost/test/unit_test.hpp>
#include <memory.hpp>

#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Doc, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Smoke)
{
    eval(R"JS(
import doc from 'noto:doc';
print(doc());
const index = doc('index');
if(!index.data.includes("doc('Storage')") ||
   !index.data.includes("doc('crypto.so.Storage')") ||
   index.data.includes("doc('noto:global.Storage')"))
    throw new Error('Global/package documentation references are incorrect');
print(index);

const global = doc('noto:global');
if(!global.data.includes("doc('Storage')") ||
   global.data.includes("doc('noto:global.Storage')"))
    throw new Error('Global member reference is incorrectly qualified');

const crypto = doc('crypto.so');
if(!crypto.data.includes("doc('crypto.so.Storage')") ||
   !crypto.data.includes("doc('hash')") ||
   crypto.data.includes("doc('crypto.so.hash')"))
    throw new Error('Package member reference qualification is incorrect');

const storage = doc('Storage');
const globalStorage = doc('noto:global.Storage');
const cryptoStorage = doc('crypto.so.Storage');
if(storage.data.includes('Not found') ||
   globalStorage.data.includes('Not found') ||
   cryptoStorage.data.includes('Not found'))
    throw new Error('Documentation reference not found');
if(!storage.data.includes('`noto:global`'))
    throw new Error('Unqualified Storage did not resolve to globalThis.Storage');
if(!cryptoStorage.data.includes('**`module`** `crypto.so`') ||
   cryptoStorage.data.includes('**`module`** `crypto.so.Storage`'))
    throw new Error('Qualified member has incorrect module attribution');
print(global, crypto, storage, globalStorage, cryptoStorage);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
