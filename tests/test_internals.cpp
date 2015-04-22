#include <ews/ews.hpp>
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <cstring>

namespace
{
    const std::string contact_card =
R"(<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/">
    <s:Body xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema">
        <m:GetItemResponse xmlns:m="http://schemas.microsoft.com/exchange/services/2006/messages" xmlns:t="http://schemas.microsoft.com/exchange/services/2006/types">
            <m:ResponseMessages>
                <m:GetItemResponseMessage ResponseClass="Success">
                    <m:ResponseCode>NoError</m:ResponseCode>
                    <m:Items>
                        <t:Contact>
                            <t:DateTimeSent>2015-05-21T10:13:28Z</t:DateTimeSent>
                            <t:DateTimeCreated>2015-05-21T10:13:28Z</t:DateTimeCreated>
                            <t:EffectiveRights>
                                <t:Delete>true</t:Delete>
                                <t:Modify>true</t:Modify>
                                <t:Read>true</t:Read>
                            </t:EffectiveRights>
                            <t:Culture>en-US</t:Culture>
                        </t:Contact>
                    </m:Items>
                </m:GetItemResponseMessage>
            </m:ResponseMessages>
        </m:GetItemResponse>
    </s:Body>
</s:Envelope>)";

}

namespace tests
{
    TEST(InternalTest, NamespaceURIs)
    {
        using uri = ews::internal::uri<>;

        EXPECT_EQ(uri::microsoft::errors_size,
                  std::strlen(uri::microsoft::errors()));
        EXPECT_EQ(uri::microsoft::types_size,
                  std::strlen(uri::microsoft::types()));
        EXPECT_EQ(uri::microsoft::messages_size,
                  std::strlen(uri::microsoft::messages()));

        EXPECT_EQ(uri::soapxml::envelope_size,
                  std::strlen(uri::soapxml::envelope()));
    }

    TEST(InternalTest, MimeContentDefaultConstruction)
    {
        using mime_content = ews::mime_content;

        auto m = mime_content();
        EXPECT_TRUE(m.none());
        EXPECT_EQ(0U, m.len_bytes());
        EXPECT_TRUE(m.character_set().empty());
        EXPECT_EQ(nullptr, m.bytes());
    }

    TEST(InternalTest, MimeContentConstructionWithData)
    {
        using mime_content = ews::mime_content;
        const char* content = "SGVsbG8sIHdvcmxkPw==";

        auto m = mime_content();

        {
            const auto b = std::string(content);
            m = mime_content("UTF-8", b.data(), b.length());
            EXPECT_FALSE(m.none());
            EXPECT_EQ(20U, m.len_bytes());
            EXPECT_FALSE(m.character_set().empty());
            EXPECT_STREQ("UTF-8", m.character_set().c_str());
            EXPECT_NE(nullptr, m.bytes());
        }

        // b was destructed

#if EWS_HAS_ROBUST_NONMODIFYING_SEQ_OPS
        EXPECT_TRUE(std::equal(m.bytes(), m.bytes() + m.len_bytes(),
                               content, content + std::strlen(content)));
#else
        EXPECT_TRUE(m.len_bytes() == std::strlen(content) &&
                    std::equal(m.bytes(), m.bytes() + m.len_bytes(),
                               content));
#endif
    }

    TEST(InternalTest, ExtractSubTreeFromDocument)
    {
        using namespace ews::internal;

        rapidxml::xml_document<> doc;
        auto str = doc.allocate_string(contact_card.c_str());
        doc.parse<0>(str);
        auto effective_rights_element =
            get_element_by_qname(doc,
                                 "EffectiveRights",
                                 uri<>::microsoft::types());
        auto effective_rights = item_properties(*effective_rights_element);
        EXPECT_STREQ(
            "<t:EffectiveRights><t:Delete>true</t:Delete><t:Modify>true</t:Modify><t:Read>true</t:Read></t:EffectiveRights>",
            effective_rights.to_string().c_str());
    }

    TEST(InternalTest, GetNonExistingElementReturnsNullptr)
    {
        using namespace ews::internal;

        rapidxml::xml_document<> doc;
        auto str = doc.allocate_string(
            R"(<html lang="en"><head><meta charset="utf-8"/><title>Welcome</title></head><body><h1>Greetings</h1><p>Hello!</p></body></html>)");
        doc.parse<0>(str);
        auto body_element = get_element_by_qname(doc, "body", "");
        auto body = item_properties(*body_element);
        rapidxml::xml_node<char>* node = nullptr;
        ASSERT_NO_THROW(
        {
            node = body.get_node("h1");
        });
        EXPECT_EQ(nullptr, node);
    }

    TEST(InternalTest, GetElementFromSubTree)
    {
        using namespace ews::internal;

        rapidxml::xml_document<> doc;
        auto str = doc.allocate_string(contact_card.c_str());
        doc.parse<0>(str);
        auto effective_rights_element =
            get_element_by_qname(doc,
                                 "EffectiveRights",
                                 uri<>::microsoft::types());
        auto effective_rights = item_properties(*effective_rights_element);

        EXPECT_STREQ("true", effective_rights.get_node("Delete")->value());
    }

    TEST(InternalTest, UpdateSubTreeElement)
    {
        using namespace ews::internal;

        rapidxml::xml_document<> doc;
        auto str = doc.allocate_string(contact_card.c_str());
        doc.parse<0>(str);
        auto effective_rights_element =
            get_element_by_qname(doc,
                                 "EffectiveRights",
                                 uri<>::microsoft::types());
        auto effective_rights = item_properties(*effective_rights_element);

        effective_rights.set_or_update("Modify", "false");
        EXPECT_STREQ("false", effective_rights.get_node("Modify")->value());
        EXPECT_STREQ(
            "<t:EffectiveRights><t:Delete>true</t:Delete><t:Modify>false</t:Modify><t:Read>true</t:Read></t:EffectiveRights>",
            effective_rights.to_string().c_str());
    }

    // TODO: test size_hint parameter of item_properties reduces/eliminates reallocs
}
