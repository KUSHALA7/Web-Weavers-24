/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>
#include <react/renderer/element/Element.h>
#include <react/renderer/element/testUtils.h>

using namespace facebook::react;

class LayoutableShadowNodeTest : public ::testing::TestWithParam<bool> {
 public:
  LayoutableShadowNodeTest()
      : builder_(simpleComponentBuilder(createContextContainer())) {}

  ContextContainer::Shared createContextContainer() {
    auto contextContainer = std::make_shared<ContextContainer>();
    contextContainer->insert("CalculateTransformedFramesEnabled", GetParam());
    return contextContainer;
  }

  bool getCalculateTransformedFramesEnabled() {
    return GetParam();
  }

  ComponentBuilder builder_;
};

/*
 * ┌────────┐
 * │<View>  │
 * │        │
 * │   ┌────┴───┐
 * │   │<View>  │
 * └───┤        │
 *     │        │
 *     │        │
 *     └────────┘
 */
TEST_P(LayoutableShadowNodeTest, relativeLayoutMetrics) {
  auto childShadowNode = std::shared_ptr<ViewShadowNode>{};
  // clang-format off
  auto element =
    Element<ViewShadowNode>()
      .finalize([](ViewShadowNode &shadowNode){
        auto layoutMetrics = EmptyLayoutMetrics;
        layoutMetrics.frame.origin = {10, 20};
        layoutMetrics.frame.size = {100, 200};
        shadowNode.setLayoutMetrics(layoutMetrics);
      })
      .children({
        Element<ViewShadowNode>()
        .finalize([](ViewShadowNode &shadowNode){
          auto layoutMetrics = EmptyLayoutMetrics;
          layoutMetrics.frame.origin = {10, 20};
          layoutMetrics.frame.size = {100, 200};
          shadowNode.setLayoutMetrics(layoutMetrics);
        })
        .reference(childShadowNode)
    });
  // clang-format on

  auto parentShadowNode = builder_.build(element);

  auto relativeLayoutMetrics =
      LayoutableShadowNode::computeRelativeLayoutMetrics(
          childShadowNode->getFamily(), *parentShadowNode, {});

  // A is a parent to B, A has origin {10, 10}, B has origin {10, 10}.
  // B's relative origin to A should be {10, 10}.
  // D19447900 has more about the issue.
  EXPECT_EQ(relativeLayoutMetrics.frame.size.width, 100);
  EXPECT_EQ(relativeLayoutMetrics.frame.size.height, 200);
  EXPECT_EQ(relativeLayoutMetrics.frame.origin.x, 10);
  EXPECT_EQ(relativeLayoutMetrics.frame.origin.y, 20);
}

/*
 * ┌───────────────────┐
 * │<View displayNone> │
 * │                   │
 * └───────────────────┘
 */
TEST_P(LayoutableShadowNodeTest, relativeLayoutMetricsOnNodeWithDisplayNone) {
  // clang-format off
  auto element =
    Element<ViewShadowNode>()
      .finalize([](ViewShadowNode &shadowNode){
        auto layoutMetrics = EmptyLayoutMetrics;
        layoutMetrics.frame.size = {100, 200};
        layoutMetrics.frame.origin = {10, 20};
        layoutMetrics.displayType = DisplayType::None;
        shadowNode.setLayoutMetrics(layoutMetrics);
    });
  // clang-format on

  auto shadowNode = builder_.build(element);

  auto relativeLayoutMetrics =
      LayoutableShadowNode::computeRelativeLayoutMetrics(
          shadowNode->getFamily(), *shadowNode, {});

  // The node has display: none, so the relative layout is empty.
  EXPECT_TRUE(relativeLayoutMetrics == EmptyLayoutMetrics);
}

/*
 * ┌────────────────────┐
 * │<View displayNone>  │
 * │                    │
 * │   ┌────────────────┴──┐
 * │   │<View>             │
 * └───┤                   │
 *     │                   │
 *     │                   │
 *     └───────────────────┘
 */
TEST_P(
    LayoutableShadowNodeTest,
    relativeLayoutMetricsOnChildrenOfParentWithDisplayNone) {
  auto childShadowNode = std::shared_ptr<ViewShadowNode>{};
  // clang-format off
  auto element =
    Element<ViewShadowNode>()
      .finalize([](ViewShadowNode &shadowNode){
        auto layoutMetrics = EmptyLayoutMetrics;
        layoutMetrics.frame.origin = {10, 20};
        layoutMetrics.frame.size = {100, 200};
        layoutMetrics.displayType = DisplayType::None;
        shadowNode.setLayoutMetrics(layoutMetrics);
      })
      .children({
        Element<ViewShadowNode>()
        .finalize([](ViewShadowNode &shadowNode){
          auto layoutMetrics = EmptyLayoutMetrics;
          layoutMetrics.frame.origin = {10, 20};
          layoutMetrics.frame.size = {100, 200};
          shadowNode.setLayoutMetrics(layoutMetrics);
        })
        .reference(childShadowNode)
    });
  // clang-format on

  auto parentShadowNode = builder_.build(element);

  auto relativeLayoutMetrics =
      LayoutableShadowNode::computeRelativeLayoutMetrics(
          childShadowNode->getFamily(), *parentShadowNode, {});

  // A parent node in the hierarchy has display: none, so the relative layout
  // is empty.
  EXPECT_TRUE(relativeLayoutMetrics == EmptyLayoutMetrics);
}

/*
 * ┌────────────────────┐
 * │<View displayNone>  │
 * │                    │
 * │   ┌────────────────┴──┐
 * │   │<View ancestor>    │
 * └───┤                   │
 *     │                   │
 *     │   ┌───────────────┴──┐
 *     └───|<View descendant> |
 *         |                  |
 *         └──────────────────┘
 */
TEST_P(
    LayoutableShadowNodeTest,
    relativeLayoutMetricsOnOuterParentWithDisplayNone) {
  auto parentShadowNode = std::shared_ptr<ViewShadowNode>{};
  auto childShadowNode = std::shared_ptr<ViewShadowNode>{};
  // clang-format off
  auto element =
    Element<ViewShadowNode>()
      .finalize([](ViewShadowNode &shadowNode){
        auto layoutMetrics = EmptyLayoutMetrics;
        layoutMetrics.frame.origin = {0, 0};
        layoutMetrics.frame.size = {1000, 1000};
        layoutMetrics.displayType = DisplayType::None;
        shadowNode.setLayoutMetrics(layoutMetrics);
      })
      .children({
        Element<ViewShadowNode>()
        .finalize([](ViewShadowNode &shadowNode){
          auto layoutMetrics = EmptyLayoutMetrics;
          layoutMetrics.frame.origin = {50, 50};
          layoutMetrics.frame.size = {200, 200};
          shadowNode.setLayoutMetrics(layoutMetrics);
        })
        .children({
          Element<ViewShadowNode>()
            .finalize([](ViewShadowNode &shadowNode){
              auto layoutMetrics = EmptyLayoutMetrics;
              layoutMetrics.frame.origin = {10, 20};
              layoutMetrics.frame.size = {100, 200};
              shadowNode.setLayoutMetrics(layoutMetrics);
            }).reference(childShadowNode)
        })
        .reference(parentShadowNode)
    });
  // clang-format on

  auto outerParentShadowNode = builder_.build(element);

  auto relativeLayoutMetrics =
      LayoutableShadowNode::computeRelativeLayoutMetrics(
          childShadowNode->getFamily(), *parentShadowNode, {});

  // The root view has display: none, but all the views between the descendant
  // and the ancestor are visible, so we compute relative layout as usual.
  EXPECT_EQ(relativeLayoutMetrics.frame.size.width, 100);
  EXPECT_EQ(relativeLayoutMetrics.frame.size.height, 200);
  EXPECT_EQ(relativeLayoutMetrics.frame.origin.x, 10);
  EXPECT_EQ(relativeLayoutMetrics.frame.origin.y, 20);
}

/*
 * ┌──────────────┐
 * │<ScrollView>  │
 * │        ┌─────┴───┐
 * │        │<View>   │
 * │        │         │
 * └────────┤         │
 *          │         │
 *          └─────────┘
 */
TEST_P(LayoutableShadowNodeTest, contentOriginOffset) {
  auto childShadowNode = std::shared_ptr<ViewShadowNode>{};
  // clang-format off
  auto element =
    Element<ScrollViewShadowNode>()
      .finalize([](ScrollViewShadowNode &shadowNode){
        auto layoutMetrics = EmptyLayoutMetrics;
        layoutMetrics.frame.origin = {10, 20};
        layoutMetrics.frame.size = {100, 200};
        shadowNode.setLayoutMetrics(layoutMetrics);
      })
      .stateData([](ScrollViewState &data) {
        data.contentOffset = {10, 10};
      })
      .children({
        Element<ViewShadowNode>()
        .finalize([](ViewShadowNode &shadowNode){
          auto layoutMetrics = EmptyLayoutMetrics;
          layoutMetrics.frame.origin = {10, 20};
          layoutMetrics.frame.size = {100, 200};
          shadowNode.setLayoutMetrics(layoutMetrics);
        })
        .reference(childShadowNode)
    });
  // clang-format on

  auto parentShadowNode = builder_.build(element);

  auto relativeLayoutMetrics =
      LayoutableShadowNode::computeRelativeLayoutMetrics(
          childShadowNode->getFamily(),
          *parentShadowNode,
          {/* includeTransform = */ true});

  EXPECT_EQ(relativeLayoutMetrics.frame.origin.x, 0);
  EXPECT_EQ(relativeLayoutMetrics.frame.origin.y, 10);

  relativeLayoutMetrics = LayoutableShadowNode::computeRelativeLayoutMetrics(
      childShadowNode->getFamily(),
      *parentShadowNode,
      {/* includeTransform = */ false});

  EXPECT_EQ(relativeLayoutMetrics.frame.origin.x, 10);
  EXPECT_EQ(relativeLayoutMetrics.frame.origin.y, 20);
}

/*
 * ┌────────────────────────┐
 * │<View>                  │
 * │      ┌────────────────┐│
 * │      │<View>          ││
 * │      │                ││
 * │      └────────────────┘│
 * └────────────────────────┘
 */
TEST_P(LayoutableShadowNodeTest, relativeLayoutMetricsOnTransformedNode) {
  auto childShadowNode = std::shared_ptr<ViewShadowNode>{};
  // clang-format off
  auto element =
    Element<ViewShadowNode>()
      .finalize([](ViewShadowNode &shadowNode){
        auto layoutMetrics = EmptyLayoutMetrics;
        layoutMetrics.frame.size = {1000, 1000};
        shadowNode.setLayoutMetrics(layoutMetrics);
      })
      .children({
        Element<ViewShadowNode>()
        .finalize([](ViewShadowNode &shadowNode){
          auto layoutMetrics = EmptyLayoutMetrics;
          layoutMetrics.frame.origin = {10, 20};
          layoutMetrics.frame.size = {100, 200};
          shadowNode.setLayoutMetrics(layoutMetrics);
        })
        .props([] {
          auto sharedProps = std::make_shared<ViewShadowNodeProps>();
          sharedProps->transform = Transform::Scale(0.5, 0.5, 1);
          return sharedProps;
        })
        .reference(childShadowNode)
    });
  // clang-format on

  auto parentShadowNode = builder_.build(element);

  auto relativeLayoutMetrics =
      LayoutableShadowNode::computeRelativeLayoutMetrics(
          childShadowNode->getFamily(), *parentShadowNode, {});

  EXPECT_EQ(relativeLayoutMetrics.frame.origin.x, 35);
  EXPECT_EQ(relativeLayoutMetrics.frame.origin.y, 70);
  EXPECT_EQ(relativeLayoutMetrics.frame.size.width, 50);
  EXPECT_EQ(relativeLayoutMetrics.frame.size.height, 100);
}

/*
 * ┌────────────────────────┐
 * │<Root>                  │
 * │ ┌─────────────────────┐│
 * │ │ <View>              ││
 * │ │     ┌──────────────┐││
 * │ │     │<View>        │││
 * │ │     │  ┌──────────┐│││
 * │ │     │  │<View>    ││││
 * │ │     │  │          ││││
 * │ │     │  │          ││││
 * │ │     │  └──────────┘│││
 * │ │     └──────────────┘││
 * │ └─────────────────────┘│
 * └────────────────────────┘
 */
TEST_P(LayoutableShadowNodeTest, relativeLayoutMetricsOnTransformedParent) {
  auto childShadowNode = std::shared_ptr<ViewShadowNode>{};
  // clang-format off
  auto element =
    Element<RootShadowNode>()
      .finalize([](RootShadowNode &shadowNode){
        auto layoutMetrics = EmptyLayoutMetrics;
        layoutMetrics.frame.size = {900, 900};
        shadowNode.setLayoutMetrics(layoutMetrics);
      })
      .children({
        Element<ViewShadowNode>()
        .props([] {
          auto sharedProps = std::make_shared<ViewShadowNodeProps>();
          sharedProps->transform = Transform::Scale(0.5, 0.5, 1);
          return sharedProps;
        })
        .finalize([](ViewShadowNode &shadowNode){
          auto layoutMetrics = EmptyLayoutMetrics;
          layoutMetrics.frame.origin = {10, 10};
          layoutMetrics.frame.size = {100, 100};
          shadowNode.setLayoutMetrics(layoutMetrics);
        })
        .children({
          Element<ViewShadowNode>()
          .reference(childShadowNode)
          .finalize([](ViewShadowNode &shadowNode){
            auto layoutMetrics = EmptyLayoutMetrics;
            layoutMetrics.frame.origin = {10, 10};
            layoutMetrics.frame.size = {50, 50};
            shadowNode.setLayoutMetrics(layoutMetrics);
          })
        })
    });
  // clang-format on

  auto parentShadowNode = builder_.build(element);

  auto relativeLayoutMetrics =
      LayoutableShadowNode::computeRelativeLayoutMetrics(
          childShadowNode->getFamily(), *parentShadowNode, {});

  EXPECT_EQ(relativeLayoutMetrics.frame.origin.x, 40);
  EXPECT_EQ(relativeLayoutMetrics.frame.origin.y, 40);

  EXPECT_EQ(relativeLayoutMetrics.frame.size.width, 25);
  EXPECT_EQ(relativeLayoutMetrics.frame.size.height, 25);
}

/*
 * ┌────────────────────────┐
 * │<Root>                  │
 * │ ┌─────────────────────┐│
 * │ │ <View>              ││
 * │ │     ┌──────────────┐││
 * │ │     │<View>        │││
 * │ │     │  ┌──────────┐│││
 * │ │     │  │<View>    ││││
 * │ │     │  │          ││││
 * │ │     │  │          ││││
 * │ │     │  └──────────┘│││
 * │ │     └──────────────┘││
 * │ └─────────────────────┘│
 * └────────────────────────┘
 */
TEST_P(LayoutableShadowNodeTest, relativeLayoutMetricsOnParentWithClipping) {
  auto childShadowNode = std::shared_ptr<ViewShadowNode>{};
  // clang-format off
  auto element =
    Element<RootShadowNode>()
      .finalize([](RootShadowNode &shadowNode){
        auto layoutMetrics = EmptyLayoutMetrics;
        layoutMetrics.frame.size = {900, 900};
        shadowNode.setLayoutMetrics(layoutMetrics);
      })
      .children({
        Element<ViewShadowNode>()
        .finalize([](ViewShadowNode &shadowNode){
          auto layoutMetrics = EmptyLayoutMetrics;
          layoutMetrics.frame.origin = {10, 10};
          layoutMetrics.frame.size = {100, 100};
          shadowNode.setLayoutMetrics(layoutMetrics);
        })
        .children({
          Element<ViewShadowNode>()
          .reference(childShadowNode)
          .finalize([](ViewShadowNode &shadowNode){
            auto layoutMetrics = EmptyLayoutMetrics;
            layoutMetrics.frame.origin = {10, 10};
            layoutMetrics.frame.size = {150, 150};
            shadowNode.setLayoutMetrics(layoutMetrics);
          })
        })
    });
  // clang-format on

  auto parentShadowNode = builder_.build(element);

  auto relativeLayoutMetrics =
      LayoutableShadowNode::computeRelativeLayoutMetrics(
          childShadowNode->getFamily(),
          *parentShadowNode,
          {
              /* includeTransform = */ true,
              /* includeViewportOffset = */ false,
              /* enableOverflowClipping = */ true,
          });

  EXPECT_EQ(relativeLayoutMetrics.frame.origin.x, 20);
  EXPECT_EQ(relativeLayoutMetrics.frame.origin.y, 20);

  EXPECT_EQ(relativeLayoutMetrics.frame.size.width, 90);
  EXPECT_EQ(relativeLayoutMetrics.frame.size.height, 90);
}

/*
 * ┌────────────────────────┐
 * │<Root>                  │
 * │ ┌─────────────────────┐│
 * │ │ <View>              ││
 * │ │     ┌──────────────┐││
 * │ │     │<View>        │││
 * │ │     │  ┌──────────┐│││
 * │ │     │  │<View>    ││││
 * │ │     │  │          ││││
 * │ │     │  │          ││││
 * │ │     │  └──────────┘│││
 * │ │     └──────────────┘││
 * │ └─────────────────────┘│
 * └────────────────────────┘
 */
TEST_P(
    LayoutableShadowNodeTest,
    relativeLayoutMetricsOnTransformedParentWithClipping) {
  auto childShadowNode = std::shared_ptr<ViewShadowNode>{};
  // clang-format off
  auto element =
    Element<RootShadowNode>()
      .finalize([](RootShadowNode &shadowNode){
        auto layoutMetrics = EmptyLayoutMetrics;
        layoutMetrics.frame.size = {900, 900};
        shadowNode.setLayoutMetrics(layoutMetrics);
      })
      .children({
        Element<ViewShadowNode>()
        .props([] {
          auto sharedProps = std::make_shared<ViewShadowNodeProps>();
          sharedProps->transform = Transform::Scale(0.5, 0.5, 1);
          return sharedProps;
        })
        .finalize([](ViewShadowNode &shadowNode){
          auto layoutMetrics = EmptyLayoutMetrics;
          layoutMetrics.frame.origin = {10, 10};
          layoutMetrics.frame.size = {100, 100};
          shadowNode.setLayoutMetrics(layoutMetrics);
        })
        .children({
          Element<ViewShadowNode>()
          .reference(childShadowNode)
          .finalize([](ViewShadowNode &shadowNode){
            auto layoutMetrics = EmptyLayoutMetrics;
            layoutMetrics.frame.origin = {10, 10};
            layoutMetrics.frame.size = {150, 150};
            shadowNode.setLayoutMetrics(layoutMetrics);
          })
        })
    });
  // clang-format on

  auto parentShadowNode = builder_.build(element);

  auto relativeLayoutMetrics =
      LayoutableShadowNode::computeRelativeLayoutMetrics(
          childShadowNode->getFamily(),
          *parentShadowNode,
          {
              /* includeTransform = */ true,
              /* includeViewportOffset = */ false,
              /* enableOverflowClipping = */ true,
          });

  EXPECT_EQ(relativeLayoutMetrics.frame.origin.x, 40);
  EXPECT_EQ(relativeLayoutMetrics.frame.origin.y, 40);

  EXPECT_EQ(relativeLayoutMetrics.frame.size.width, 45);
  EXPECT_EQ(relativeLayoutMetrics.frame.size.height, 45);
}

/*
 * ┌────────────────┐
 * │<View>          │
 * │                │
 * └────────────────┘
 */
TEST_P(LayoutableShadowNodeTest, relativeLayoutMetricsOnSameNode) {
  // clang-format off
  auto element =
    Element<ViewShadowNode>()
      .finalize([](ViewShadowNode &shadowNode){
        auto layoutMetrics = EmptyLayoutMetrics;
        layoutMetrics.frame.size = {100, 200};
        layoutMetrics.frame.origin = {10, 20};
        shadowNode.setLayoutMetrics(layoutMetrics);
    });
  // clang-format on

  auto shadowNode = builder_.build(element);

  auto relativeLayoutMetrics =
      LayoutableShadowNode::computeRelativeLayoutMetrics(
          shadowNode->getFamily(), *shadowNode, {});

  EXPECT_EQ(relativeLayoutMetrics.frame.origin.x, 0);
  EXPECT_EQ(relativeLayoutMetrics.frame.origin.y, 0);
  EXPECT_EQ(relativeLayoutMetrics.frame.size.width, 100);
  EXPECT_EQ(relativeLayoutMetrics.frame.size.height, 200);
}

/*
 * ┌────────────────┐
 * │<View>          │
 * │                │
 * └────────────────┘
 */
TEST_P(LayoutableShadowNodeTest, relativeLayoutMetricsOnSameTransformedNode) {
  // clang-format off
  auto element =
    Element<ViewShadowNode>()
      .props([] {
        auto sharedProps = std::make_shared<ViewShadowNodeProps>();
        sharedProps->transform = Transform::Scale(2, 2, 1);
        return sharedProps;
      })
      .finalize([](ViewShadowNode &shadowNode){
        auto layoutMetrics = EmptyLayoutMetrics;
        layoutMetrics.frame.size = {100, 200};
        layoutMetrics.frame.origin = {10, 20};
        shadowNode.setLayoutMetrics(layoutMetrics);
    });
  // clang-format on

  auto shadowNode = builder_.build(element);

  auto relativeLayoutMetrics =
      LayoutableShadowNode::computeRelativeLayoutMetrics(
          shadowNode->getFamily(), *shadowNode, {});

  EXPECT_EQ(relativeLayoutMetrics.frame.origin.x, 0);
  EXPECT_EQ(relativeLayoutMetrics.frame.origin.y, 0);
  EXPECT_EQ(relativeLayoutMetrics.frame.size.width, 200);
  EXPECT_EQ(relativeLayoutMetrics.frame.size.height, 400);
}

/*
 * ┌────────────────────────┐
 * │<View>                  │
 * │      ┌────────────────┐│
 * │      │<View>          ││
 * │      │                ││
 * │      └────────────────┘│
 * └────────────────────────┘
 */
TEST_P(LayoutableShadowNodeTest, relativeLayoutMetricsOnClonedNode) {
  auto childShadowNode = std::shared_ptr<ViewShadowNode>{};

  // clang-format off
  auto element =
    Element<ViewShadowNode>()
      .children({
        Element<ViewShadowNode>()
          .reference(childShadowNode)
      });
  // clang-format on

  auto parentShadowNode = builder_.build(element);

  auto clonedChildShadowNode =
      std::static_pointer_cast<ViewShadowNode>(childShadowNode->clone({}));
  auto layoutMetrics = EmptyLayoutMetrics;
  layoutMetrics.frame.size = {50, 60};
  clonedChildShadowNode->setLayoutMetrics(layoutMetrics);

  parentShadowNode->replaceChild(*childShadowNode, clonedChildShadowNode);

  auto newRelativeLayoutMetrics =
      LayoutableShadowNode::computeRelativeLayoutMetrics(
          childShadowNode->getFamily(), *parentShadowNode, {});
  EXPECT_EQ(newRelativeLayoutMetrics.frame.size.width, 50);
  EXPECT_EQ(newRelativeLayoutMetrics.frame.size.height, 60);
}

/*
 * ┌─────────────────────────┐
 * │<View>                   │
 * │ ┌──────────────────────┐│
 * │ │<Modal>               ││
 * │ │     ┌───────────┐    ││
 * │ │     │<View>     │    ││
 * │ │     │           │    ││
 * │ │     └───────────┘    ││
 * │ └──────────────────────┘│
 * └─────────────────────────┘
 */
TEST_P(
    LayoutableShadowNodeTest,
    relativeLayoutMetricsOnNodesCrossingRootKindNode) {
  auto childShadowNode = std::shared_ptr<ViewShadowNode>{};

  // clang-format off
  auto element =
    Element<ViewShadowNode>()
      .children({
        Element<ModalHostViewShadowNode>()
          .finalize([](ModalHostViewShadowNode &shadowNode){
            auto layoutMetrics = EmptyLayoutMetrics;
            layoutMetrics.frame.origin = {10, 10};
            shadowNode.setLayoutMetrics(layoutMetrics);
          })
          .children({
            Element<ViewShadowNode>()
            .reference(childShadowNode)
            .finalize([](ViewShadowNode &shadowNode){
              auto layoutMetrics = EmptyLayoutMetrics;
              layoutMetrics.frame.origin = {10, 10};
              shadowNode.setLayoutMetrics(layoutMetrics);
            })
          })
      });

  auto parentShadowNode = builder_.build(element);

  auto relativeLayoutMetrics = LayoutableShadowNode::computeRelativeLayoutMetrics(childShadowNode->getFamily(), *parentShadowNode, {});

  // relativeLayoutMetrics do not include offset of nodeAA_ because it is a
  // RootKindNode.
  EXPECT_EQ(relativeLayoutMetrics.frame.origin.x, 10);
  EXPECT_EQ(relativeLayoutMetrics.frame.origin.y, 10);
}

TEST_P(LayoutableShadowNodeTest, includeViewportOffset) {

  auto viewShadowNode = std::shared_ptr<ViewShadowNode>{};

  // clang-format off
  auto element =
      Element<RootShadowNode>()
        .props([] {
          auto sharedProps = std::make_shared<RootProps>();
          sharedProps->layoutContext.viewportOffset = {10, 20};
          return sharedProps;
        })
        .children({
          Element<ViewShadowNode>()
          .reference(viewShadowNode)
        });
  // clang-format on

  auto rootShadowNode = builder_.build(element);

  // `includeViewportOffset` has to work with `includeTransform` enabled and
  // disabled.
  auto layoutMetrics = LayoutableShadowNode::computeRelativeLayoutMetrics(
      viewShadowNode->getFamily(),
      *rootShadowNode,
      {/* includeTransform = */ false, /* includeViewportOffset = */ true});
  EXPECT_EQ(layoutMetrics.frame.origin.x, 10);
  EXPECT_EQ(layoutMetrics.frame.origin.y, 20);

  layoutMetrics = LayoutableShadowNode::computeRelativeLayoutMetrics(
      viewShadowNode->getFamily(),
      *rootShadowNode,
      {/* includeTransform = */ true, /* includeViewportOffset = */ true});
  EXPECT_EQ(layoutMetrics.frame.origin.x, 10);
  EXPECT_EQ(layoutMetrics.frame.origin.y, 20);
}

/*
 * ┌───────────────────────────────┐
 * │ <View verticallyInverted>     │
 * │                               │
 * │┌─────────────────────────────┐│
 * ││<View childShadowNode1>      ││
 * ││                             ││
 * │└─────────────────────────────┘│
 * │┌─────────────────────────────┐│
 * ││<View childShadowNode2>      ││
 * ││                             ││
 * │└─────────────────────────────┘│
 * └───────────────────────────────┘
 */
TEST_P(LayoutableShadowNodeTest, invertedVerticalView) {
  auto childShadowNode1 = std::shared_ptr<ViewShadowNode>{};
  auto childShadowNode2 = std::shared_ptr<ViewShadowNode>{};

  // clang-format off
  auto element =
        Element<ViewShadowNode>()
          .props([] {
            auto sharedProps = std::make_shared<ViewShadowNodeProps>();
            sharedProps->transform = Transform::VerticalInversion(); // Inverted <ScrollView>
            return sharedProps;
          })
            .finalize([](ViewShadowNode &shadowNode){
              auto layoutMetrics = EmptyLayoutMetrics;
              layoutMetrics.frame.size = {200, 200};
              shadowNode.setLayoutMetrics(layoutMetrics);
            }).children({
                Element<ViewShadowNode>()
                  .reference(childShadowNode1)
                  .finalize([](ViewShadowNode &shadowNode){
                    auto layoutMetrics = EmptyLayoutMetrics;
                    layoutMetrics.frame.origin = {0, 0};
                    layoutMetrics.frame.size = {100, 100};
                    shadowNode.setLayoutMetrics(layoutMetrics);
                  }),
                Element<ViewShadowNode>()
                  .reference(childShadowNode2)
                  .finalize([](ViewShadowNode &shadowNode){
                    auto layoutMetrics = EmptyLayoutMetrics;
                    layoutMetrics.frame.origin = {0, 100};
                    layoutMetrics.frame.size = {100, 100};
                    shadowNode.setLayoutMetrics(layoutMetrics);
                  })
        });
  // clang-format on

  auto scrollShadowNode = builder_.build(element);

  auto firstItemRelativeLayoutMetrics =
      LayoutableShadowNode::computeRelativeLayoutMetrics(
          childShadowNode1->getFamily(), *scrollShadowNode, {});

  if (getCalculateTransformedFramesEnabled()) {
    EXPECT_EQ(firstItemRelativeLayoutMetrics.frame.origin.y, 0);
  } else {
    EXPECT_EQ(firstItemRelativeLayoutMetrics.frame.origin.y, 100);
  }

  EXPECT_EQ(firstItemRelativeLayoutMetrics.frame.origin.x, 0);
  EXPECT_EQ(firstItemRelativeLayoutMetrics.frame.size.width, 100);
  EXPECT_EQ(firstItemRelativeLayoutMetrics.frame.size.height, 100);

  auto secondItemRelativeLayoutMetrics =
      LayoutableShadowNode::computeRelativeLayoutMetrics(
          childShadowNode2->getFamily(), *scrollShadowNode, {});

  if (getCalculateTransformedFramesEnabled()) {
    EXPECT_EQ(secondItemRelativeLayoutMetrics.frame.origin.y, 100);
  } else {
    EXPECT_EQ(secondItemRelativeLayoutMetrics.frame.origin.y, 0);
  }

  EXPECT_EQ(secondItemRelativeLayoutMetrics.frame.origin.x, 0);
  EXPECT_EQ(secondItemRelativeLayoutMetrics.frame.size.width, 100);
  EXPECT_EQ(secondItemRelativeLayoutMetrics.frame.size.height, 100);
}

/*
 * ┌────────────────────────────────────┐
 * │ <View verticallyInverted>          │
 * │                                    │
 * │ ┌───────────────────────────────┐  │
 * │ │ <View>                        │  │
 * │ │                               │  │
 * │ │┌─────────────────────────────┐│  │
 * │ ││<View childShadowNode1>      ││  │
 * │ ││                             ││  │
 * │ │└─────────────────────────────┘│  │
 * │ │┌─────────────────────────────┐│  │
 * │ ││<View childShadowNode2>      ││  │
 * │ ││                             ││  │
 * │ │└─────────────────────────────┘│  │
 * │ └───────────────────────────────┘  │
 * └────────────────────────────────────┘
 */
TEST_P(LayoutableShadowNodeTest, nestedInvertedVerticalView) {
  auto childShadowNode1 = std::shared_ptr<ViewShadowNode>{};
  auto childShadowNode2 = std::shared_ptr<ViewShadowNode>{};

  // clang-format off
  auto element =
    Element<ViewShadowNode>()
      .props([] {
        auto sharedProps = std::make_shared<ViewShadowNodeProps>();
        sharedProps->transform = Transform::VerticalInversion(); // Inverted <ScrollView>
        return sharedProps;
      })
      .finalize([](ViewShadowNode &shadowNode){
        auto layoutMetrics = EmptyLayoutMetrics;
        layoutMetrics.frame.size = {400, 400};
        shadowNode.setLayoutMetrics(layoutMetrics);
    })
      .children({
        Element<ViewShadowNode>()
              .finalize([](ViewShadowNode &shadowNode){
                auto layoutMetrics = EmptyLayoutMetrics;
                layoutMetrics.frame.origin = {100, 50};
                layoutMetrics.frame.size = {200, 200};
                shadowNode.setLayoutMetrics(layoutMetrics);
              }).children({
                Element<ViewShadowNode>()
                  .reference(childShadowNode1)
                  .finalize([](ViewShadowNode &shadowNode){
                    auto layoutMetrics = EmptyLayoutMetrics;
                    layoutMetrics.frame.origin = {0, 0};
                    layoutMetrics.frame.size = {100, 100};
                    shadowNode.setLayoutMetrics(layoutMetrics);
                  }),
                Element<ViewShadowNode>()
                  .reference(childShadowNode2)
                  .finalize([](ViewShadowNode &shadowNode){
                    auto layoutMetrics = EmptyLayoutMetrics;
                    layoutMetrics.frame.origin = {0, 100};
                    layoutMetrics.frame.size = {100, 100};
                    shadowNode.setLayoutMetrics(layoutMetrics);
                  })
        })
      });
  // clang-format on

  auto scrollShadowNode = builder_.build(element);

  auto firstItemRelativeLayoutMetrics =
      LayoutableShadowNode::computeRelativeLayoutMetrics(
          childShadowNode1->getFamily(), *scrollShadowNode, {});

  if (getCalculateTransformedFramesEnabled()) {
    EXPECT_EQ(firstItemRelativeLayoutMetrics.frame.origin.y, 50);
  } else {
    EXPECT_EQ(firstItemRelativeLayoutMetrics.frame.origin.y, 250);
  }

  EXPECT_EQ(firstItemRelativeLayoutMetrics.frame.origin.x, 100);
  EXPECT_EQ(firstItemRelativeLayoutMetrics.frame.size.width, 100);
  EXPECT_EQ(firstItemRelativeLayoutMetrics.frame.size.height, 100);

  auto secondItemRelativeLayoutMetrics =
      LayoutableShadowNode::computeRelativeLayoutMetrics(
          childShadowNode2->getFamily(), *scrollShadowNode, {});

  EXPECT_EQ(secondItemRelativeLayoutMetrics.frame.origin.x, 100);
  EXPECT_EQ(secondItemRelativeLayoutMetrics.frame.origin.y, 150);
  EXPECT_EQ(secondItemRelativeLayoutMetrics.frame.size.width, 100);
  EXPECT_EQ(secondItemRelativeLayoutMetrics.frame.size.height, 100);
}

/*
 * ┌──────────────────────────────────────┐
 * │ <View horizontallyInverted>          │
 * │                                      │
 * │┌─────────────────┐┌─────────────────┐│
 * ││ <View>          ││ <View>          ││
 * ││                 ││                 ││
 * │└─────────────────┘└─────────────────┘│
 * └──────────────────────────────────────┘
 */
TEST_P(LayoutableShadowNodeTest, invertedHorizontalView) {
  auto childShadowNode1 = std::shared_ptr<ViewShadowNode>{};
  auto childShadowNode2 = std::shared_ptr<ViewShadowNode>{};

  // clang-format off
  auto element =
        Element<ViewShadowNode>()
          .props([] {
            auto sharedProps = std::make_shared<ViewShadowNodeProps>();
            sharedProps->transform = Transform::HorizontalInversion(); // Inverted <ScrollView>
            return sharedProps;
          })
            .finalize([](ViewShadowNode &shadowNode){
              auto layoutMetrics = EmptyLayoutMetrics;
              layoutMetrics.frame.size = {200, 200};
              shadowNode.setLayoutMetrics(layoutMetrics);
            }).children({
                Element<ViewShadowNode>()
                  .reference(childShadowNode1)
                  .finalize([](ViewShadowNode &shadowNode){
                    auto layoutMetrics = EmptyLayoutMetrics;
                    layoutMetrics.frame.origin = {0, 0};
                    layoutMetrics.frame.size = {100, 100};
                    shadowNode.setLayoutMetrics(layoutMetrics);
                  }),
                Element<ViewShadowNode>()
                  .reference(childShadowNode2)
                  .finalize([](ViewShadowNode &shadowNode){
                    auto layoutMetrics = EmptyLayoutMetrics;
                    layoutMetrics.frame.origin = {100, 0};
                    layoutMetrics.frame.size = {100, 100};
                    shadowNode.setLayoutMetrics(layoutMetrics);
                  })
        });
  // clang-format on

  auto scrollShadowNode = builder_.build(element);

  auto firstItemRelativeLayoutMetrics =
      LayoutableShadowNode::computeRelativeLayoutMetrics(
          childShadowNode1->getFamily(), *scrollShadowNode, {});

  if (getCalculateTransformedFramesEnabled()) {
    EXPECT_EQ(firstItemRelativeLayoutMetrics.frame.origin.x, 0);
  } else {
    // Incorrect legacy behaviour
    EXPECT_EQ(firstItemRelativeLayoutMetrics.frame.origin.x, 100);
  }

  EXPECT_EQ(firstItemRelativeLayoutMetrics.frame.origin.y, 0);
  EXPECT_EQ(firstItemRelativeLayoutMetrics.frame.size.width, 100);
  EXPECT_EQ(firstItemRelativeLayoutMetrics.frame.size.height, 100);

  auto secondItemRelativeLayoutMetrics =
      LayoutableShadowNode::computeRelativeLayoutMetrics(
          childShadowNode2->getFamily(), *scrollShadowNode, {});
  if (getCalculateTransformedFramesEnabled()) {
    EXPECT_EQ(secondItemRelativeLayoutMetrics.frame.origin.x, 100);
  } else {
    // Incorrect legacy behaviour
    EXPECT_EQ(secondItemRelativeLayoutMetrics.frame.origin.x, 0);
  }
  EXPECT_EQ(secondItemRelativeLayoutMetrics.frame.origin.y, 0);
  EXPECT_EQ(secondItemRelativeLayoutMetrics.frame.size.width, 100);
  EXPECT_EQ(secondItemRelativeLayoutMetrics.frame.size.height, 100);
}

/*
 * ┌──────────────────────────────────────────┐
 * │ <View horizontallyInverted>              │
 * │                                          │
 * │ ┌──────────────────────────────────────┐ │
 * │ │ <View>                               │ │
 * │ │                                      │ │
 * │ │┌─────────────────┐┌─────────────────┐│ │
 * │ ││ <View>          ││ <View>          ││ │
 * │ ││                 ││                 ││ │
 * │ │└─────────────────┘└─────────────────┘│ │
 * │ └──────────────────────────────────────┘ │
 * └──────────────────────────────────────────┘
 */
TEST_P(LayoutableShadowNodeTest, nestedInvertedHorizontalView) {
  auto childShadowNode1 = std::shared_ptr<ViewShadowNode>{};
  auto childShadowNode2 = std::shared_ptr<ViewShadowNode>{};

  // clang-format off
  auto element =
    Element<ViewShadowNode>()
      .props([] {
        auto sharedProps = std::make_shared<ViewShadowNodeProps>();
        sharedProps->transform = Transform::HorizontalInversion(); // Inverted <ScrollView>
        return sharedProps;
      })
      .finalize([](ViewShadowNode &shadowNode){
        auto layoutMetrics = EmptyLayoutMetrics;
        layoutMetrics.frame.size = {400, 400};
        shadowNode.setLayoutMetrics(layoutMetrics);
    })
      .children({
        Element<ViewShadowNode>()
              .finalize([](ViewShadowNode &shadowNode){
                auto layoutMetrics = EmptyLayoutMetrics;
                layoutMetrics.frame.origin = {50, 100};
                layoutMetrics.frame.size = {200, 200};
                shadowNode.setLayoutMetrics(layoutMetrics);
              }).children({
                Element<ViewShadowNode>()
                  .reference(childShadowNode1)
                  .finalize([](ViewShadowNode &shadowNode){
                    auto layoutMetrics = EmptyLayoutMetrics;
                    layoutMetrics.frame.origin = {0, 0};
                    layoutMetrics.frame.size = {100, 100};
                    shadowNode.setLayoutMetrics(layoutMetrics);
                  }),
                Element<ViewShadowNode>()
                  .reference(childShadowNode2)
                  .finalize([](ViewShadowNode &shadowNode){
                    auto layoutMetrics = EmptyLayoutMetrics;
                    layoutMetrics.frame.origin = {100, 0};
                    layoutMetrics.frame.size = {100, 100};
                    shadowNode.setLayoutMetrics(layoutMetrics);
                  })
        })
      });
  // clang-format on

  auto scrollShadowNode = builder_.build(element);

  auto firstItemRelativeLayoutMetrics =
      LayoutableShadowNode::computeRelativeLayoutMetrics(
          childShadowNode1->getFamily(), *scrollShadowNode, {});

  if (getCalculateTransformedFramesEnabled()) {
    EXPECT_EQ(firstItemRelativeLayoutMetrics.frame.origin.x, 50);
  } else {
    // Incorrect legacy behaviour
    EXPECT_EQ(firstItemRelativeLayoutMetrics.frame.origin.x, 250);
  }
  EXPECT_EQ(firstItemRelativeLayoutMetrics.frame.origin.y, 100);
  EXPECT_EQ(firstItemRelativeLayoutMetrics.frame.size.width, 100);
  EXPECT_EQ(firstItemRelativeLayoutMetrics.frame.size.height, 100);

  auto secondItemRelativeLayoutMetrics =
      LayoutableShadowNode::computeRelativeLayoutMetrics(
          childShadowNode2->getFamily(), *scrollShadowNode, {});

  EXPECT_EQ(secondItemRelativeLayoutMetrics.frame.origin.x, 150);
  EXPECT_EQ(secondItemRelativeLayoutMetrics.frame.origin.y, 100);
  EXPECT_EQ(secondItemRelativeLayoutMetrics.frame.size.width, 100);
  EXPECT_EQ(secondItemRelativeLayoutMetrics.frame.size.height, 100);
}

TEST_P(LayoutableShadowNodeTest, inversedContentOriginOffset) {
  auto childShadowNode = std::shared_ptr<ViewShadowNode>{};

  // clang-format off
  auto element =
    Element<ScrollViewShadowNode>()
      .props([] {
        auto sharedProps = std::make_shared<ScrollViewProps>();
        sharedProps->transform = Transform::HorizontalInversion() * Transform::VerticalInversion();
        return sharedProps;
      })
      .finalize([](ScrollViewShadowNode &shadowNode){
        auto layoutMetrics = EmptyLayoutMetrics;
        layoutMetrics.frame.size = {300, 350};
        shadowNode.setLayoutMetrics(layoutMetrics);
      })
      .stateData([](ScrollViewState &data) {
        data.contentOffset = {10, 20};
      })
      .children({
        Element<ViewShadowNode>()
        .finalize([](ViewShadowNode &shadowNode){
          auto layoutMetrics = EmptyLayoutMetrics;
          layoutMetrics.frame.origin = {30, 40};
          layoutMetrics.frame.size = {100, 200};
          shadowNode.setLayoutMetrics(layoutMetrics);
        })
        .reference(childShadowNode)
    });
  // clang-format on

  auto parentShadowNode = builder_.build(element);

  auto relativeLayoutMetrics =
      LayoutableShadowNode::computeRelativeLayoutMetrics(
          childShadowNode->getFamily(), *parentShadowNode, {});

  if (getCalculateTransformedFramesEnabled()) {
    EXPECT_EQ(relativeLayoutMetrics.frame.origin.x, 20);
    EXPECT_EQ(relativeLayoutMetrics.frame.origin.y, 20);
  } else {
    // Incorrect legacy behaviour
    EXPECT_EQ(relativeLayoutMetrics.frame.origin.x, 160);
    EXPECT_EQ(relativeLayoutMetrics.frame.origin.y, 90);
  }
}

INSTANTIATE_TEST_SUITE_P(
    CalculateTransformedFrames,
    LayoutableShadowNodeTest,
    testing::Values(false, true));
